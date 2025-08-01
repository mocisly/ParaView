// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageVolumeRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkContourValues.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiBlockVolumeMapper.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPVLODVolume.h"
#include "vtkPVRenderView.h"
#include "vtkPVTransferFunction2D.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRectilinearGrid.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredData.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVolumeProperty.h"

#include <algorithm>
#include <string>

namespace
{
//----------------------------------------------------------------------------
void vtkGetNonGhostExtent(int* resultExtent, vtkImageData* dataSet)
{
  // this is really only meant for topologically structured grids
  dataSet->GetExtent(resultExtent);

  if (vtkUnsignedCharArray* ghostArray = vtkUnsignedCharArray::SafeDownCast(
        dataSet->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName())))
  {
    // We have a ghost array. We need to iterate over the array to prune ghost
    // extents.

    int pntExtent[6];
    std::copy(resultExtent, resultExtent + 6, pntExtent);

    int validCellExtent[6];
    vtkStructuredData::GetCellExtentFromPointExtent(pntExtent, validCellExtent);

    // The start extent is the location of the first cell with ghost value 0.
    for (vtkIdType cc = 0, numTuples = ghostArray->GetNumberOfTuples(); cc < numTuples; ++cc)
    {
      if (ghostArray->GetValue(cc) == 0)
      {
        int ijk[3];
        vtkStructuredData::ComputeCellStructuredCoordsForExtent(cc, pntExtent, ijk);
        validCellExtent[0] = ijk[0];
        validCellExtent[2] = ijk[1];
        validCellExtent[4] = ijk[2];
        break;
      }
    }

    // The end extent is the  location of the last cell with ghost value 0.
    for (vtkIdType cc = (ghostArray->GetNumberOfTuples() - 1); cc >= 0; --cc)
    {
      if (ghostArray->GetValue(cc) == 0)
      {
        int ijk[3];
        vtkStructuredData::ComputeCellStructuredCoordsForExtent(cc, pntExtent, ijk);
        validCellExtent[1] = ijk[0];
        validCellExtent[3] = ijk[1];
        validCellExtent[5] = ijk[2];
        break;
      }
    }

    // convert cell-extents to pt extents.
    resultExtent[0] = validCellExtent[0];
    resultExtent[2] = validCellExtent[2];
    resultExtent[4] = validCellExtent[4];

    resultExtent[1] = std::min(validCellExtent[1] + 1, resultExtent[1]);
    resultExtent[3] = std::min(validCellExtent[3] + 1, resultExtent[3]);
    resultExtent[5] = std::min(validCellExtent[5] + 1, resultExtent[5]);
  }
}
}

vtkStandardNewMacro(vtkImageVolumeRepresentation);
//----------------------------------------------------------------------------
vtkImageVolumeRepresentation::vtkImageVolumeRepresentation()
{
  this->VolumeMapper.TakeReference(vtkMultiBlockVolumeMapper::New());

  this->Actor->SetLODMapper(this->OutlineMapper);
}

//----------------------------------------------------------------------------
vtkImageVolumeRepresentation::~vtkImageVolumeRepresentation()
{
  this->TransferFunction2D = nullptr;
}

//----------------------------------------------------------------------------
int vtkImageVolumeRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageVolumeRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }
  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    // pass the actual volumetric data.
    vtkPVRenderView::SetPiece(inInfo, this, this->Cache, this->DataSize, 0);

    // We never want the volumetric data to be delivered to the client.
    vtkPVRenderView::SetDeliverToClientAndRenderingProcesses(inInfo, this,
      /*deliver_to_client*/ false,
      /*gather_before_delivery*/ false, 0);

    // pass the outline data, used on ranks where the data may not be available.
    vtkPVRenderView::SetPiece(inInfo, this, this->OutlineSource->GetOutputDataObject(0), 0, 1);

    // BUG #14792.
    // We report this->DataSize explicitly since the data being "delivered" is
    // not the data that should be used to make rendering decisions based on
    // data size.
    outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);

    vtkPVRenderView::SetGeometryBounds(inInfo, this, this->DataBounds);

    vtkPVRenderView::SetRequiresDistributedRendering(inInfo, this, true);
    // Pass partitioning information to the render view.
    vtkPVRenderView::SetOrderedCompositingConfiguration(
      inInfo, this, vtkPVRenderView::USE_BOUNDS_FOR_REDISTRIBUTION);
  }
  else if (request_type == vtkPVView::REQUEST_UPDATE_LOD())
  {
    vtkPVRenderView::SetRequiresDistributedRenderingLOD(inInfo, this, true);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    auto volumeProducer = vtkPVRenderView::GetPieceProducer(inInfo, this, 0);
    this->VolumeMapper->SetInputConnection(volumeProducer);
    this->UpdateMapperParameters();

    vtkAlgorithmOutput* outlineProducer = vtkPVRenderView::GetPieceProducer(inInfo, this, 1);
    this->OutlineMapper->SetInputConnection(outlineProducer);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageVolumeRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMath::UninitializeBounds(this->DataBounds);
  this->DataSize = 0;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    if (auto inputID = vtkImageData::GetData(inputVector[0], 0))
    {
      vtkSmartPointer<vtkImageData> cache = nullptr;
      if (auto inputUG = vtkUniformGrid::GetData(inputVector[0], 0))
      {
        cache = vtkSmartPointer<vtkUniformGrid>::New();
        cache->ShallowCopy(inputUG);

        if (this->UseSeparateOpacityArray)
        {
          this->AppendOpacityComponent(cache);
        }
      }
      else
      {
        cache = vtkSmartPointer<vtkImageData>::New();
        cache->ShallowCopy(inputID);

        if (this->UseSeparateOpacityArray)
        {
          this->AppendOpacityComponent(cache);
        }
      }
      if (inputID->HasAnyGhostCells())
      {
        int ext[6];
        vtkGetNonGhostExtent(ext, cache);
        // Yup, this will modify the "input", but that okay for now. Ultimately,
        // we will teach the volume mapper to handle ghost cells and this won't
        // be needed. Once that's done, we'll need to teach the KdTree
        // generation code to handle overlap in extents, however.
        cache->Crop(ext);
      }

      this->Actor->SetEnableLOD(0);
      this->VolumeMapper->SetInputData(cache);

      this->OutlineSource->SetBounds(cache->GetBounds());
      this->OutlineSource->GetBounds(this->DataBounds);
      this->OutlineSource->Update();
      this->DataSize = cache->GetActualMemorySize();
      vtkStreamingDemandDrivenPipeline::GetWholeExtent(
        inputVector[0]->GetInformationObject(0), this->WholeExtent);
      this->Cache = cache.GetPointer();
    }
    else if (auto inputRD = vtkRectilinearGrid::GetData(inputVector[0], 0))
    {
      vtkNew<vtkRectilinearGrid> cache;
      cache->ShallowCopy(inputRD);

      if (this->UseSeparateOpacityArray)
      {
        this->AppendOpacityComponent(cache);
      }

      this->Actor->SetEnableLOD(0);
      this->VolumeMapper->SetInputData(cache);

      this->OutlineSource->SetBounds(cache->GetBounds());
      this->OutlineSource->GetBounds(this->DataBounds);
      this->OutlineSource->Update();
      this->DataSize = cache->GetActualMemorySize();
      vtkStreamingDemandDrivenPipeline::GetWholeExtent(
        inputVector[0]->GetInformationObject(0), this->WholeExtent);
      this->Cache = cache.GetPointer();
    }
    else if (auto inputPD = vtkPartitionedDataSet::GetData(inputVector[0], 0))
    {
      if (!vtkMultiBlockVolumeMapper::SafeDownCast(this->VolumeMapper))
      {
        vtkWarningMacro("Representation does not support rendering partitioned datasets yet.");
      }
      else
      {
        vtkNew<vtkPartitionedDataSet> cache;
        cache->CopyStructure(inputPD);
        for (unsigned int cc = 0; cc < inputPD->GetNumberOfPartitions(); ++cc)
        {
          auto partition = inputPD->GetPartition(cc);
          if (this->UseSeparateOpacityArray)
          {
            this->AppendOpacityComponent(partition);
          }
          auto partitionID = vtkImageData::SafeDownCast(partition);
          auto partitionRG = vtkRectilinearGrid::SafeDownCast(partition);
          if (partitionID)
          {
            cache->SetPartition(cc, partitionID);
          }
          else
          {
            cache->SetPartition(cc, partitionRG);
          }
        }
        this->Cache = cache.GetPointer();

        cache->GetBounds(this->DataBounds);
        this->OutlineSource->SetBounds(this->DataBounds);
        this->OutlineSource->Update();
        this->DataSize = cache->GetActualMemorySize();
      }
    }
    else if (auto doTree = vtkDataObjectTree::GetData(inputVector[0], 0))
    {
      auto images = vtkCompositeDataSet::GetDataSets(doTree);
      images.erase(std::remove_if(images.begin(), images.end(),
                     [](vtkDataSet* ds)
                     {
                       return vtkRectilinearGrid::SafeDownCast(ds) == nullptr &&
                         vtkImageData::SafeDownCast(ds) == nullptr;
                     }),
        images.end());

      vtkNew<vtkPartitionedDataSet> cache;
      for (unsigned int cc = 0; cc < static_cast<unsigned int>(images.size()); ++cc)
      {
        auto* partition = images[cc];
        if (this->UseSeparateOpacityArray)
        {
          this->AppendOpacityComponent(partition);
        }
        cache->SetPartition(cc, partition);
      }

      this->Cache = cache.GetPointer();

      cache->GetBounds(this->DataBounds);
      this->OutlineSource->SetBounds(this->DataBounds);
      this->OutlineSource->Update();
      this->DataSize = cache->GetActualMemorySize();
    }
  }
  else
  {
    // We just need an empty data object on the client side
    // so that the pipelines can update properly. Since we never deliver
    // this data to the client, we don't have to worry too much about the types
    // matching. Just creating a vtkPartitionedDataSet does the trick
    this->Cache = vtk::TakeSmartPointer(vtkPartitionedDataSet::New());

    // when no input is present, it implies that this processes is on a node
    // without the data input i.e. either client or render-server, in which case
    // we show only the outline.
    this->VolumeMapper->RemoveAllInputs();
    this->Actor->SetEnableLOD(1);
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkImageVolumeRepresentation::AddToView(vtkView* view)
{
  // FIXME: Need generic view API to add props.
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->AddActor(this->Actor);

    // Indicate that the above renderer is the one the actor is relative to
    // in case the coordinate system is set to physical or device.
    this->Actor->SetCoordinateSystemRenderer(rview->GetRenderer());

    // Indicate that this is a prop to be rendered during hardware selection.
    rview->RegisterPropForHardwareSelection(this, this->GetRenderedProp());

    return this->Superclass::AddToView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkImageVolumeRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    this->Actor->SetCoordinateSystemRenderer(nullptr);
    rview->GetRenderer()->RemoveActor(this->Actor);
    return this->Superclass::RemoveFromView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::UpdateMapperParameters()
{
  const char* colorArrayName = nullptr;
  int fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;

  vtkInformation* info = this->GetInputArrayInformation(0);
  if (info && info->Has(vtkDataObject::FIELD_ASSOCIATION()) &&
    info->Has(vtkDataObject::FIELD_NAME()))
  {
    colorArrayName = info->Get(vtkDataObject::FIELD_NAME());
    fieldAssociation = info->Get(vtkDataObject::FIELD_ASSOCIATION());
  }

  if (this->UseSeparateOpacityArray)
  {
    // See AppendOpacityComponent() for the construction of this array.
    // FIXME: `colorArrayName` might be `nullptr` here.
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.StringChecker)
    std::string combinedName(colorArrayName);
    combinedName += "_and_opacity";
    this->VolumeMapper->SelectScalarArray(combinedName.c_str());
  }
  else
  {
    this->VolumeMapper->SelectScalarArray(colorArrayName);
  }
  switch (fieldAssociation)
  {
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      this->VolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
      break;

    case vtkDataObject::FIELD_ASSOCIATION_NONE:
      this->VolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_FIELD_DATA);
      break;

    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
    default:
      this->VolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
      break;
  }

  this->Actor->SetMapper(this->VolumeMapper);
  // this is necessary since volume mappers don't like empty arrays.
  this->Actor->SetVisibility(colorArrayName != nullptr && colorArrayName[0] != 0);

  if (this->VolumeMapper->GetCropping())
  {
    double planes[6];
    for (int i = 0; i < 6; i++)
    {
      planes[i] = this->CroppingOrigin[i / 2] + this->WholeExtent[i] * this->CroppingScale[i / 2];
    }
    this->VolumeMapper->SetCroppingRegionPlanes(planes);
  }

  if (this->Property)
  {
    if (this->MapScalars)
    {
      if (this->MultiComponentsMapping || this->UseSeparateOpacityArray)
      {
        this->Property->SetIndependentComponents(false);
      }
      else
      {
        this->Property->SetIndependentComponents(true);
      }
    }
    else
    {
      this->Property->SetIndependentComponents(false);
    }

    if (this->UseTransfer2D)
    {
      this->Property->SetTransferFunctionMode(vtkVolumeProperty::TF_2D);
      if (auto mbMapper = vtkMultiBlockVolumeMapper::SafeDownCast(this->VolumeMapper))
      {
        if (!this->UseGradientForTransfer2D && !this->ColorArray2Name.empty())
        {
          mbMapper->SetTransfer2DYAxisArray(this->ColorArray2Name.c_str());
        }
        else
        {
          mbMapper->SetTransfer2DYAxisArray(nullptr);
        }
      }
    }
    else
    {
      this->Property->SetTransferFunctionMode(vtkVolumeProperty::TF_1D);
      // Update the mapper's vector mode
      vtkColorTransferFunction* ctf = this->Property->GetRGBTransferFunction(0);

      // When vtkScalarsToColors::MAGNITUDE mode is active, vtkSmartVolumeMapper
      // uses an internally generated (single-component) dataset.  However,
      // unchecking MapScalars (e.g. IndependentComponents == 0) requires 2C or 4C
      // data. In that case, vtkScalarsToColors::COMPONENT is forced in order to
      // make vtkSmartVolumeMapper use the original multiple-component dataset.
      int const indep = this->Property->GetIndependentComponents();
      int const mode = indep ? ctf->GetVectorMode() : vtkScalarsToColors::COMPONENT;
      int const comp = indep ? ctf->GetVectorComponent() : 0;

      if (auto smartVolumeMapper = vtkSmartVolumeMapper::SafeDownCast(this->VolumeMapper))
      {
        smartVolumeMapper->SetVectorMode(mode);
        smartVolumeMapper->SetVectorComponent(comp);
      }
      else if (auto mbMapper = vtkMultiBlockVolumeMapper::SafeDownCast(this->VolumeMapper))
      {
        mbMapper->SetVectorMode(mode);
        mbMapper->SetVectorComponent(comp);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Cropping Origin: " << this->CroppingOrigin[0] << ", " << this->CroppingOrigin[1]
     << ", " << this->CroppingOrigin[2] << endl;
  os << indent << "Cropping Scale: " << this->CroppingScale[0] << ", " << this->CroppingScale[1]
     << ", " << this->CroppingScale[2] << endl;
  os << indent << "UseTransfer2D: " << this->UseTransfer2D << endl;
  os << indent << "UseGradientForTransfer2D: " << this->UseGradientForTransfer2D << endl;
  os << indent << "ColorArray2Name: " << this->ColorArray2Name << endl;
  os << indent << "ColorArray2FieldAssociation: " << this->ColorArray2FieldAssociation << endl;
  os << indent << "ColorArray2Component: " << this->ColorArray2Component << endl;
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetAmbient(double val)
{
  this->Property->SetAmbient(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetDiffuse(double val)
{
  this->Property->SetDiffuse(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetSpecular(double val)
{
  this->Property->SetSpecular(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetSpecularPower(double val)
{
  this->Property->SetSpecularPower(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetShade(bool val)
{
  this->Property->SetShade(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetAnisotropy(float val)
{
  this->Property->SetScatteringAnisotropy(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetCoordinateSystem(int coordSys)
{
  this->Actor->SetCoordinateSystem(static_cast<vtkProp3D::CoordinateSystems>(coordSys));
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetGlobalIlluminationReach(float val)
{
  if (auto smartVolumeMapper = vtkSmartVolumeMapper::SafeDownCast(this->VolumeMapper))
  {
    smartVolumeMapper->SetGlobalIlluminationReach(val);
  }
  else if (auto mbMapper = vtkMultiBlockVolumeMapper::SafeDownCast(this->VolumeMapper))
  {
    mbMapper->SetGlobalIlluminationReach(val);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetVolumetricScatteringBlending(float val)
{
  if (auto smartVolumeMapper = vtkSmartVolumeMapper::SafeDownCast(this->VolumeMapper))
  {
    smartVolumeMapper->SetVolumetricScatteringBlending(val);
  }
  else if (auto mbMapper = vtkMultiBlockVolumeMapper::SafeDownCast(this->VolumeMapper))
  {
    mbMapper->SetVolumetricScatteringBlending(val);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetSliceFunction(vtkImplicitFunction* slice)
{
  this->Property->SetSliceFunction(slice);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetRequestedRenderMode(int mode)
{
  if (auto smartVolumeMapper = vtkSmartVolumeMapper::SafeDownCast(this->VolumeMapper))
  {
    smartVolumeMapper->SetRequestedRenderMode(mode);
  }
  else if (auto mbMapper = vtkMultiBlockVolumeMapper::SafeDownCast(this->VolumeMapper))
  {
    mbMapper->SetRequestedRenderMode(mode);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetBlendMode(int blend)
{
  this->VolumeMapper->SetBlendMode(static_cast<vtkVolumeMapper::BlendModes>(blend));
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetCropping(int crop)
{
  this->VolumeMapper->SetCropping(crop != 0);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetIsosurfaceValue(int i, double value)
{
  this->Property->GetIsoSurfaceValues()->SetValue(i, value);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetNumberOfIsosurfaces(int number)
{
  this->Property->GetIsoSurfaceValues()->SetNumberOfContours(number);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetUseTransfer2D(bool value)
{
  if (this->UseTransfer2D != value)
  {
    this->UseTransfer2D = value;
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetUseGradientForTransfer2D(bool value)
{
  if (this->UseGradientForTransfer2D != value)
  {
    this->UseGradientForTransfer2D = value;
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SelectColorArray2(
  int, int, int, int fieldAssociation, const char* name)
{
  std::string newName;
  if (name)
  {
    newName = std::string(name);
  }

  if (this->ColorArray2Name != newName)
  {
    this->ColorArray2Name = newName;
    this->MarkModified();
  }

  if (this->ColorArray2FieldAssociation != fieldAssociation)
  {
    this->ColorArray2FieldAssociation = fieldAssociation;
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SelectColorArray2Component(int component)
{
  if (this->ColorArray2Component != component)
  {
    this->ColorArray2Component = component;
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetTransferFunction2D(vtkPVTransferFunction2D* transfer2D)
{
  if (this->TransferFunction2D == nullptr && transfer2D == nullptr)
  {
    return;
  }
  if (this->TransferFunction2D && transfer2D && this->TransferFunction2D == transfer2D)
  {
    return;
  }

  if (this->TransferFunction2D)
  {
    this->TransferFunction2D = nullptr;
    this->Property->SetTransferFunction2D(nullptr);
  }

  if (transfer2D)
  {
    this->TransferFunction2D = transfer2D;
    vtkImageData* func = this->TransferFunction2D->GetFunction();
    if (func)
    {
      vtkPointData* pd = func->GetPointData();
      if (!pd || (pd->GetScalars() == nullptr) || pd->GetScalars()->GetNumberOfComponents() != 4)
      {
        int* dims = this->TransferFunction2D->GetOutputDimensions();
        func->SetDimensions(dims[0], dims[1], 1);
        func->AllocateScalars(VTK_FLOAT, 4);
      }
      this->Property->SetTransferFunction2D(func);
    }
    this->MarkModified();
  }
}

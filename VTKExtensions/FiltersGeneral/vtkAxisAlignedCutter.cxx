// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAxisAlignedCutter.h"
#include "vtkCommunicator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkConvertToPartitionedDataSetCollection.h"
#include "vtkDataAssembly.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDummyController.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridAxisCut.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPlane.h"
#include "vtkSmartPointer.h"
#include "vtkType.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAxisAlignedCutter);

//----------------------------------------------------------------------------
vtkAxisAlignedCutter::vtkAxisAlignedCutter()
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
void vtkAxisAlignedCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->CutFunction != nullptr)
  {
    os << indent << "CutFunction: " << endl;
    this->CutFunction->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "CutFunction: (nullptr)" << endl;
  }

  os << indent << "OffsetValues: " << endl;
  this->OffsetValues->PrintSelf(os, indent.GetNextIndent());

  os << indent << "LevelOfResolution: " << LevelOfResolution << endl;

  os << indent << "HTGCutter: " << endl;
  this->HTGCutter->PrintSelf(os, indent.GetNextIndent());

  os << indent << "AMRCutter: " << endl;
  this->AMRCutter->PrintSelf(os, indent.GetNextIndent());

  if (this->Controller != nullptr)
  {
    os << indent << "Controller: " << endl;
    this->Controller->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "Controller: (nullptr)" << endl;
  }
}

//------------------------------------------------------------------------------
vtkMTimeType vtkAxisAlignedCutter::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->CutFunction != nullptr)
  {
    vtkMTimeType planeMTime = this->CutFunction->GetMTime();
    return (planeMTime > mTime ? planeMTime : mTime);
  }

  return mTime;
}

//------------------------------------------------------------------------------
void vtkAxisAlignedCutter::SetCutFunction(vtkImplicitFunction* function)
{
  vtkSetSmartPointerBodyMacro(CutFunction, vtkImplicitFunction, function);
}

//------------------------------------------------------------------------------
vtkImplicitFunction* vtkAxisAlignedCutter::GetCutFunction()
{
  return this->CutFunction;
}

//----------------------------------------------------------------------------
void vtkAxisAlignedCutter::SetOffsetValue(int i, double value)
{
  this->OffsetValues->SetValue(i, value);
  this->Modified();
}

//----------------------------------------------------------------------------
double vtkAxisAlignedCutter::GetOffsetValue(int i)
{
  return this->OffsetValues->GetValue(i);
}

//----------------------------------------------------------------------------
void vtkAxisAlignedCutter::SetNumberOfOffsetValues(int number)
{
  this->OffsetValues->SetNumberOfContours(number);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkAxisAlignedCutter::GetNumberOfOffsetValues()
{
  return this->OffsetValues->GetNumberOfContours();
}

//----------------------------------------------------------------------------
void vtkAxisAlignedCutter::SetController(vtkMultiProcessController* controller)
{
  vtkSetSmartPointerBodyMacro(Controller, vtkMultiProcessController, controller);
  if (!this->Controller)
  {
    this->Controller = vtkSmartPointer<vtkDummyController>::New();
  }
}

//------------------------------------------------------------------------------
vtkMultiProcessController* vtkAxisAlignedCutter::GetController()
{
  return this->Controller;
}

//----------------------------------------------------------------------------
int vtkAxisAlignedCutter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkOverlappingAMR* inputAMR = vtkOverlappingAMR::GetData(inInfo);
  if (inputAMR)
  {
    // Input is an AMR and can't be stored in a vtkPartitionedDataSetCollection,
    // so output is a vtkOverlappingAMR (single slice support only)
    bool outputAlreadyCreated = vtkOverlappingAMR::GetData(outInfo);
    if (!outputAlreadyCreated)
    {
      vtkNew<vtkOverlappingAMR> outputAMR;
      this->GetExecutive()->SetOutputData(0, outputAMR);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), outputAMR->GetExtentType());
    }
    return 1;
  }

  // Check we have a valid composite input (should only contain HTGs)
  vtkDataObjectTree* inputComposite = vtkDataObjectTree::GetData(inInfo);
  if (inputComposite)
  {
    vtkSmartPointer<vtkDataObjectTreeIterator> iter;
    iter.TakeReference(inputComposite->NewTreeIterator());
    iter->VisitOnlyLeavesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      // AMRs cannot be contained in a composite dataset
      if (vtkHyperTreeGrid::SafeDownCast(iter->GetCurrentDataObject()))
      {
        continue;
      }

      vtkErrorMacro(
        "Input composite dataset should only contain vtkHyperTreeGrid instances as leaves.");
      return 0;
    }
  }

  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::GetData(inInfo);
  if (inputHTG || inputComposite)
  {
    // Input dataset support can be stored in a composite output (multi-slice available)
    // so output is a vtkPartitionedDataSetCollection of vtkHyperTreeGrid
    bool outputAlreadyCreated = vtkPartitionedDataSetCollection::GetData(outInfo);
    if (!outputAlreadyCreated)
    {
      vtkNew<vtkPartitionedDataSetCollection> outputPDC;
      this->GetExecutive()->SetOutputData(0, outputPDC);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), outputPDC->GetExtentType());
    }
    return 1;
  }

  vtkErrorMacro("Unable to retrieve input as vtkOverlappingAMR, vtkHyperTreeGrid or composite "
                "dataset of vtkHyperTreeGrid instances.");
  return 0;
}

//----------------------------------------------------------------------------
int vtkAxisAlignedCutter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkPlane* plane = vtkPlane::SafeDownCast(this->CutFunction);
  if (plane && plane->GetAxisAligned())
  {
    vtkOverlappingAMR* inputAMR = vtkOverlappingAMR::GetData(inInfo);
    if (inputAMR)
    {
      // vtkOverlappingAMR only support one slice
      vtkOverlappingAMR* outputAMR = vtkOverlappingAMR::GetData(outInfo);
      if (!outputAMR)
      {
        vtkErrorMacro(<< "Unable to retrieve output as vtkOverlappingAMR.");
        return 0;
      }

      this->CutAMRWithAAPlane(inputAMR, outputAMR, plane);
      return 1;
    }

    vtkDataObjectTree* inputComposite = vtkDataObjectTree::GetData(inInfo);
    if (inputComposite)
    {
      vtkPartitionedDataSetCollection* outputPDC =
        vtkPartitionedDataSetCollection::GetData(outInfo);
      if (!outputPDC)
      {
        vtkErrorMacro(<< "Unable to retrieve output as vtkPartitionedDataSetCollection.");
        return 0;
      }

      // Convert composite input to PDC if needed
      vtkNew<vtkConvertToPartitionedDataSetCollection> converter;
      converter->SetInputDataObject(inputComposite);
      converter->Update();
      vtkPartitionedDataSetCollection* inputPDC = converter->GetOutput();

      if (!inputPDC)
      {
        vtkErrorMacro(<< "Unable to convert composite input to Partitioned DataSet Collection");
        return 0;
      }

      outputPDC->Initialize();

      vtkDataAssembly* inputHierarchy = inputPDC->GetDataAssembly();
      if (inputHierarchy)
      {
        // Keep the same structure. In place of each node pointing to a HTG
        // in the input PDC, we have one new layer of node(s) pointing to slice(s)
        // (since we can have multiple slices generated at once).
        vtkNew<vtkDataAssembly> outputHierarchy;
        outputHierarchy->DeepCopy(inputHierarchy);

        std::vector<int> assemblyIndices = inputHierarchy->GetChildNodes(
          inputHierarchy->GetRootNode(), true, vtkDataAssembly::TraversalOrder::DepthFirst);

        for (auto& nodeId : assemblyIndices)
        {
          auto indices = inputHierarchy->GetDataSetIndices(nodeId, /*traverse_subtree*/ false);
          if (indices.empty())
          {
            continue;
          }

          outputHierarchy->RemoveAllDataSetIndices(nodeId, false);

          for (auto index : indices)
          {
            auto inputPDS = inputPDC->GetPartitionedDataSet(index);
            if (!this->ProcessPDS(inputPDS, plane, outputPDC, outputHierarchy, nodeId))
            {
              vtkErrorMacro(
                "Unable to process partitioned dataset at index " + std::to_string(index));
            }
          }
        }

        outputPDC->SetDataAssembly(outputHierarchy);
      }
      else
      {
        // Create a new assembly. We have one node per HTG (level 1), and one node for each
        // slice under each HTG (level 2).
        vtkNew<vtkDataAssembly> outputHierarchy;
        int rootId = outputHierarchy->GetRootNode();

        std::string rootNodeName = "AxisAlignedSlice";
        outputHierarchy->SetRootNodeName(rootNodeName.c_str());

        unsigned int hyperTreeGridCount = 0;
        for (unsigned int pdsIdx = 0; pdsIdx < inputPDC->GetNumberOfPartitionedDataSets(); pdsIdx++)
        {
          std::string htgNodeName = "HyperTreeGrid" + std::to_string(++hyperTreeGridCount);
          int htgNodeId = outputHierarchy->AddNode(htgNodeName.c_str(), rootId);
          if (htgNodeId == -1)
          {
            vtkErrorMacro("Unable to a add new child node for node " + std::to_string(rootId));
            continue;
          }

          vtkPartitionedDataSet* inputPDS = inputPDC->GetPartitionedDataSet(pdsIdx);
          if (!this->ProcessPDS(inputPDS, plane, outputPDC, outputHierarchy, htgNodeId))
          {
            vtkErrorMacro(
              "Unable to process partitioned dataset at index " + std::to_string(pdsIdx));
          }
        }
        outputPDC->SetDataAssembly(outputHierarchy);
      }

      return 1;
    }

    vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::GetData(inInfo);
    if (inputHTG)
    {
      // vtkHyperTreeGrid support multi-slice, result is stored in a vtkPartitionedDataSetCollection
      vtkPartitionedDataSetCollection* pdcOutput =
        vtkPartitionedDataSetCollection::GetData(outInfo);
      if (!pdcOutput)
      {
        vtkErrorMacro(<< "Unable to retrieve output as vtkPartitionedDataSetCollection.");
        return 0;
      }

      if (!this->ProcessHTG(inputHTG, plane, pdcOutput))
      {
        vtkErrorMacro(<< "Unable to process the input HTG.");
        return 0;
      }

      return 1;
    }

    vtkErrorMacro("Wrong input type, expected to be a vtkOverlappingAMR, vtkHyperTreeGrid or "
                  "composite dataset of vtkHyperTreeGrid instances.");
    return 0;
  }

  vtkErrorMacro("Unable to retrieve valid axis-aligned implicit function to cut with.");
  return 0;
}

//----------------------------------------------------------------------------
int vtkAxisAlignedCutter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkOverlappingAMR");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
bool vtkAxisAlignedCutter::ProcessPDS(vtkPartitionedDataSet* inputPDS, vtkPlane* plane,
  vtkPartitionedDataSetCollection* outputPDC, vtkDataAssembly* outputHierarchy, int nodeId)
{
  if (!inputPDS)
  {
    vtkErrorMacro("Unable to retrieve input partitioned dataset");
    return false;
  }

  for (int offsetIdx = 0, nbInserted = 0; offsetIdx < this->GetNumberOfOffsetValues(); offsetIdx++)
  {
    double offset = this->GetOffsetValue(offsetIdx);

    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(inputPDS->GetNumberOfPartitions());

    // Using integer for MPI communication
    int intersects = 0;

    for (unsigned int partIdx = 0; partIdx < inputPDS->GetNumberOfPartitions(); partIdx++)
    {
      vtkDataObject* partition = inputPDS->GetPartitionAsDataObject(partIdx);
      if (!partition)
      {
        // Some partitions can be empty in a distributed environment.
        continue;
      }

      auto inputHTG = vtkHyperTreeGrid::SafeDownCast(partition);
      if (!inputHTG)
      {
        vtkErrorMacro("Partition "
          << partIdx << " of input partitioned dataset should contain a HTG instance.");
        return false;
      }

      vtkNew<vtkHyperTreeGrid> outputHTG;
      this->CutHTGWithAAPlane(inputHTG, outputHTG, plane, offset);
      pds->SetPartition(partIdx, outputHTG);

      if (outputHTG && outputHTG->GetNumberOfCells() != 0)
      {
        intersects = 1;
      }
    }

    // If the plane does not intersect the PDS (HTG) on any process,
    // no need to add the slice to the output.
    int atLeastOneIntersects = 0;
    if (!this->Controller->AllReduce(
          &intersects, &atLeastOneIntersects, 1, vtkCommunicator::LOGICAL_OR_OP))
    {
      vtkErrorMacro(<< "An error ocurred during the parallel reduction operation checking that "
                       "axis-aligned plane intersection occured or not for each rank.");
      return false;
    }

    if (!atLeastOneIntersects)
    {
      continue;
    }

    auto nextDataSetId = outputPDC->GetNumberOfPartitionedDataSets();
    outputPDC->SetPartitionedDataSet(nextDataSetId, pds);

    const std::string sliceNodeName = "Slice" + std::to_string(nbInserted + 1);
    int sliceNodeId = outputHierarchy->AddNode(sliceNodeName.c_str(), nodeId);
    outputHierarchy->AddDataSetIndex(sliceNodeId, nextDataSetId);
    nbInserted++;
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkAxisAlignedCutter::ProcessHTG(
  vtkHyperTreeGrid* inputHTG, vtkPlane* plane, vtkPartitionedDataSetCollection* outputSlices)
{
  for (int offsetIdx = 0, nbInserted = 0; offsetIdx < this->GetNumberOfOffsetValues(); offsetIdx++)
  {
    vtkNew<vtkHyperTreeGrid> outputHTG;
    double offset = this->GetOffsetValue(offsetIdx);

    this->CutHTGWithAAPlane(inputHTG, outputHTG, plane, offset);

    // Using integer for MPI communication
    int intersects = 0;
    if (outputHTG && outputHTG->GetNumberOfCells() != 0)
    {
      intersects = 1;
    }

    // If the plane does not intersect the HTG on any process, no need to add the slice to the
    // output.
    int atLeastOneIntersects = 0;
    if (!this->Controller->AllReduce(
          &intersects, &atLeastOneIntersects, 1, vtkCommunicator::LOGICAL_OR_OP))
    {
      vtkErrorMacro(<< "An error ocurred during the parallel reduction operation checking that "
                       "axis-aligned plane intersection occured or not for each rank.");
      return false;
    }

    if (!atLeastOneIntersects)
    {
      continue;
    }

    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(1);
    pds->SetPartition(0, outputHTG);
    outputSlices->SetPartitionedDataSet(nbInserted, pds);
    const std::string sliceName = "Slice" + std::to_string(nbInserted);
    outputSlices->GetMetaData(nbInserted)->Set(vtkCompositeDataSet::NAME(), sliceName);
    nbInserted++;
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkAxisAlignedCutter::CutHTGWithAAPlane(
  vtkHyperTreeGrid* input, vtkHyperTreeGrid* output, vtkPlane* plane, double offset)
{
  double normal[3] = { 0., 0., 0. };
  plane->GetNormal(normal);

  int planeNormalAxis = 0;
  if (normal[1] > normal[0])
  {
    planeNormalAxis = 1;
  }
  if (normal[2] > normal[0])
  {
    planeNormalAxis = 2;
  }

  vtkNew<vtkPlane> newPlane;
  newPlane->DeepCopy(plane);
  // We should not use `Push` here since it does not apply on
  // the internal plane of vtkPVPlane
  newPlane->SetOffset(plane->GetOffset() + offset);

  this->HTGCutter->SetPlanePosition(-newPlane->EvaluateFunction(0, 0, 0));
  this->HTGCutter->SetPlaneNormalAxis(planeNormalAxis);
  this->HTGCutter->SetInputData(input);
  this->HTGCutter->Update();

  output->ShallowCopy(this->HTGCutter->GetOutput());
}

//----------------------------------------------------------------------------
void vtkAxisAlignedCutter::CutAMRWithAAPlane(
  vtkOverlappingAMR* input, vtkOverlappingAMR* output, vtkPlane* plane)
{
  double* normal = plane->GetNormal();
  double* origin = plane->GetOrigin();
  int planeNormalAxis = vtkAMRSliceFilter::X_NORMAL;
  if (normal[1] > normal[0])
  {
    planeNormalAxis = vtkAMRSliceFilter::Y_NORMAL;
  }
  if (normal[2] > normal[0])
  {
    planeNormalAxis = vtkAMRSliceFilter::Z_NORMAL;
  }

  double bounds[6] = { 0. };
  input->GetBounds(bounds);

  double offset = 0;
  switch (planeNormalAxis)
  {
    case vtkAMRSliceFilter::X_NORMAL:
      offset = origin[0] - bounds[0];
      break;
    case vtkAMRSliceFilter::Y_NORMAL:
      offset = origin[1] - bounds[2];
      break;
    case vtkAMRSliceFilter::Z_NORMAL:
    default:
      offset = origin[2] - bounds[4];
      break;
  }

  this->AMRCutter->SetOffsetFromOrigin(plane->GetOffset() + offset);
  this->AMRCutter->SetNormal(planeNormalAxis);
  this->AMRCutter->SetMaxResolution(this->LevelOfResolution);
  this->AMRCutter->SetInputData(input);
  this->AMRCutter->Update();

  output->ShallowCopy(this->AMRCutter->GetOutput());
}
VTK_ABI_NAMESPACE_END

// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVRenderView
 * @brief   Render View for ParaView.
 *
 * vtkRenderView equivalent that is specialized for ParaView. vtkRenderView
 * handles polygonal rendering for ParaView in all the different modes of
 * operation. vtkPVRenderView instance must be created on all involved
 * processes. vtkPVRenderView uses the information about what process it has
 * been created on to decide what part of the "rendering" happens on the
 * process.
 */

#ifndef vtkPVRenderView_h
#define vtkPVRenderView_h

#include "vtkBoundingBox.h" // needed for iVar
#include "vtkNew.h"         // needed for iVar
#include "vtkPVView.h"
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSmartPointer.h"        // needed for iVar
#include "vtkWeakPointer.h"         // needed for iVar

class vtkAlgorithmOutput;
class vtkCamera;
class vtkCameraOrientationWidget;
class vtkCuller;
class vtkExtentTranslator;
class vtkFloatArray;
class vtkFXAAOptions;
class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInteractorStyleDrawPolygon;
class vtkInteractorStyleRubberBand3D;
class vtkInteractorStyleRubberBandZoom;
class vtkLegendScaleActor;
class vtkLight;
class vtkLightKit;
class vtkMatrix4x4;
class vtkOrderedCompositingHelper;
class vtkPolarAxesActor2D;
class vtkProp;
class vtkPVAxesWidget;
class vtkPVCameraCollection;
class vtkPVCenterAxesActor;
class vtkPVDataRepresentation;
class vtkPVGridAxes3DActor;
class vtkPVHardwareSelector;
class vtkPVInteractorStyle;
class vtkPVMaterialLibrary;
class vtkPVSynchronizedRenderer;
class vtkRenderer;
class vtkRenderViewBase;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkSkybox;
class vtkTextRepresentation;
class vtkTexture;
class vtkTimerLog;
class vtkWindowToImageFilter;

class VTKREMOTINGVIEWS_EXPORT vtkPVRenderView : public vtkPVView
{
  //*****************************************************************
public:
  static vtkPVRenderView* New();
  vtkTypeMacro(vtkPVRenderView, vtkPVView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum InteractionModes
  {
    INTERACTION_MODE_UNINTIALIZED = -1,
    INTERACTION_MODE_3D = 0,
    INTERACTION_MODE_2D, // not implemented yet.
    INTERACTION_MODE_SELECTION,
    INTERACTION_MODE_ZOOM,
    INTERACTION_MODE_POLYGON
  };

  ///@{
  /**
   * Get/Set the interaction mode. Default is INTERACTION_MODE_3D. If
   * INTERACTION_MODE_SELECTION is
   * selected, then whenever the user drags and creates a selection region, this
   * class will fire a vtkCommand::SelectionChangedEvent event with the
   * selection region as the argument.
   * \note CallOnAllProcesses
   * \note this must be called on all processes, however it will
   * have any effect only the driver processes i.e. the process with the
   * interactor.
   */
  virtual void SetInteractionMode(int mode);
  vtkGetMacro(InteractionMode, int);
  ///@}

  ///@{
  /**
   * Overridden to call InvalidateCachedSelection() whenever the render window
   * parameters change.
   */
  void SetSize(int, int) override;
  void SetPosition(int, int) override;
  ///@}

  ///@{
  /**
   * Gets the non-composited renderer for this view. This is typically used for
   * labels, 2D annotations etc.
   * \note CallOnAllProcesses
   */
  vtkGetObjectMacro(NonCompositedRenderer, vtkRenderer);
  ///@}

  /**
   * Defines various renderer types.
   */
  enum
  {
    DEFAULT_RENDERER = 0,
    NON_COMPOSITED_RENDERER = 1,
  };

  /**
   * Returns the renderer given an int identifying its type.
   * \li DEFAULT_RENDERER: returns the 3D renderer.
   * \li NON_COMPOSITED_RENDERER: returns the NonCompositedRenderer.
   */
  virtual vtkRenderer* GetRenderer(int rendererType = DEFAULT_RENDERER);

  ///@{
  /**
   * Get/Set the active camera. The active camera is set on both the composited
   * and non-composited renderer.
   */
  vtkCamera* GetActiveCamera();
  virtual void SetActiveCamera(vtkCamera*);
  ///@}

  /**
   * Returns the interactor.
   */
  vtkRenderWindowInteractor* GetInteractor();

  /**
   * Set the interactor. Client applications must set the interactor to enable
   * interactivity. Note this method will also change the interactor styles set
   * on the interactor.
   */
  virtual void SetupInteractor(vtkRenderWindowInteractor*);

  ///@{
  /**
   * Returns the interactor style.
   */
  vtkGetObjectMacro(InteractorStyle, vtkPVInteractorStyle);
  ///@}

  ///@{
  /**
   * Resets the active camera using collective prop-bounds.
   *
   * OffsetRatio can be used to add a zoom offset.
   * \note CallOnAllProcesses
   */
  void ResetCamera();
  void ResetCamera(double bounds[6]);
  void ResetCameraScreenSpace(double offsetRatio = 0.9);
  void ResetCameraScreenSpace(double* bounds, double offsetRatio = 0.9);
  // void ResetCameraScreenSpace(double offsetRatio, double bounds[6]);
  ///@}

  ///@{
  /**
   * Compute and reduce the visible bounds of the provided
   * representation. Uses LastRepresentationVisibleBounds
   * to store the result until the next call.
   * \note CallOnAllProcesses
   */
  double* ComputeVisibleBounds(vtkPVDataRepresentation* pvrepr) VTK_SIZEHINT(6);
  double LastRepresentationVisibleBounds[6];
  ///@}

  /**
   * Triggers a high-resolution render.
   * \note Can be called on processes involved in rendering i.e those returned
   * by `this->GetStillRenderProcesses()`.
   */
  void StillRender() override;

  /**
   * Triggers a interactive render. Based on the settings on the view, this may
   * result in a low-resolution rendering or a simplified geometry rendering.
   * \note Can be called on processes involved in rendering i.e those returned
   * by `this->GetInteractiveRenderProcesses()`.
   */
  void InteractiveRender() override;

  ///@{
  /**
   * SuppressRendering can be used to suppress the render within a StillRender
   * or InteractiveRender. This is useful in cases where you want the
   * representations mappers to be setup for rendering and have their data ready
   * but not actually do the render. For example if you want to export the scene
   * but not render it you must turn on SuppressRendering and then call
   * StillRender
   */
  vtkSetMacro(SuppressRendering, bool);
  vtkGetMacro(SuppressRendering, bool);
  vtkBooleanMacro(SuppressRendering, bool);
  ///@}

  ///@{
  /**
   * Get/Set the reduction-factor to use when for StillRender(). This is
   * typically set to 1, but in some cases with terrible connectivity or really
   * large displays, one may want to use a sub-sampled image even for
   * StillRender(). This is set it number of pixels to be sub-sampled by.
   * Note that image reduction factors have no effect when in built-in mode.
   * \note CallOnAllProcesses
   */
  vtkSetClampMacro(StillRenderImageReductionFactor, int, 1, 20);
  vtkGetMacro(StillRenderImageReductionFactor, int);
  ///@}

  ///@{
  /**
   * Get/Set the reduction-factor to use when for InteractiveRender().
   * This is set it number of pixels to be sub-sampled by.
   * Note that image reduction factors have no effect when in built-in mode.
   * \note CallOnAllProcesses
   */
  vtkSetClampMacro(InteractiveRenderImageReductionFactor, int, 1, 20);
  vtkGetMacro(InteractiveRenderImageReductionFactor, int);
  ///@}

  ///@{
  /**
   * Get/Set the data-size in megabytes above which remote-rendering should be
   * used, if possible.
   * \note CallOnAllProcesses
   */
  vtkSetMacro(RemoteRenderingThreshold, double);
  vtkGetMacro(RemoteRenderingThreshold, double);
  ///@}

  ///@{
  /**
   * Get/Set the data-size in megabytes above which LOD rendering should be
   * used, if possible.
   * \note CallOnAllProcesses
   */
  vtkSetMacro(LODRenderingThreshold, double);
  vtkGetMacro(LODRenderingThreshold, double);
  ///@}

  ///@{
  /**
   * Get/Set the LOD resolution. This affects the size of the grid used for
   * quadric clustering, for example. 1.0 implies maximum resolution while 0
   * implies minimum resolution.
   * \note CallOnAllProcesses
   */
  vtkSetClampMacro(LODResolution, double, 0.0, 1.0);
  vtkGetMacro(LODResolution, double);
  ///@}

  ///@{
  /**
   * When set to true, instead of using simplified geometry for LOD rendering,
   * uses outline, if possible. Note that not all representations support this
   * mode and hence one may still see non-outline data being rendering when this
   * flag is ON and LOD is being used.
   */
  vtkSetMacro(UseOutlineForLODRendering, bool);
  vtkGetMacro(UseOutlineForLODRendering, bool);
  ///@}

  /**
   * Passes the compressor configuration to the client-server synchronizer, if
   * any. This affects the image compression used to relay images back to the
   * client.
   * See vtkPVClientServerSynchronizedRenderers::ConfigureCompressor() for
   * details.
   * \note CallOnAllProcesses
   */
  void ConfigureCompressor(const char* configuration);

  /**
   * Resets the clipping range. One does not need to call this directly ever. It
   * is called periodically by the vtkRenderer to reset the camera range.
   */
  virtual void ResetCameraClippingRange();

  ///@{
  /**
   * Enable/Disable light kit.
   * \note CallOnAllProcesses
   */
  void SetUseLightKit(bool enable);
  vtkGetMacro(UseLightKit, bool);
  vtkBooleanMacro(UseLightKit, bool);
  ///@}

  ///@{
  void StreamingUpdate(const double view_planes[24]);
  void DeliverStreamedPieces(unsigned int size, unsigned int* representation_ids);
  ///@}

  /**
   * USE_LOD indicates if LOD is being used for the current render/update.
   */
  static vtkInformationIntegerKey* USE_LOD();

  /**
   * Indicates the LOD resolution in REQUEST_UPDATE_LOD() pass.
   */
  static vtkInformationDoubleKey* LOD_RESOLUTION();

  /**
   * Indicates the LOD must use outline if possible in REQUEST_UPDATE_LOD()
   * pass.
   */
  static vtkInformationIntegerKey* USE_OUTLINE_FOR_LOD();

  /**
   * Representation can publish this key in their REQUEST_INFORMATION()
   * pass to indicate that the representation needs to disable
   * IceT's empty image optimization. This is typically only needed
   * if a painter will make use of MPI global collective communications.
   */
  static vtkInformationIntegerKey* RENDER_EMPTY_IMAGES();

  /**
   * Representation can publish this key in their REQUEST_INFORMATION() pass to
   * indicate that the representation needs ordered compositing.
   */
  static vtkInformationIntegerKey* NEED_ORDERED_COMPOSITING();

  /**
   * Key used to pass meta-data about the view frustum in REQUEST_STREAMING_UPDATE()
   * pass. The value is a double vector with exactly 24 values.
   */
  static vtkInformationDoubleVectorKey* VIEW_PLANES();

  /**
   * Streaming pass request.
   */
  static vtkInformationRequestKey* REQUEST_STREAMING_UPDATE();

  /**
   * Pass to relay the streamed "piece" to the representations.
   */
  static vtkInformationRequestKey* REQUEST_PROCESS_STREAMED_PIECE();

  ///@{
  /**
   * Make a selection. This will result in setting up of this->LastSelection
   * which can be accessed using GetLastSelection().
   * \note This method is called on call rendering processes and client (or
   * driver). Thus, if doing client only rendering, this shouldn't be called on
   * server nodes.
   */
  void SelectCells(int region[4], const char* array = nullptr);
  void SelectCells(int region0, int region1, int region2, int region3, const char* array = nullptr)
  {
    int r[4] = { region0, region1, region2, region3 };
    this->SelectCells(r, array);
  }
  void SelectPoints(int region[4], const char* array = nullptr);
  void SelectPoints(int region0, int region1, int region2, int region3, const char* array = nullptr)
  {
    int r[4] = { region0, region1, region2, region3 };
    this->SelectPoints(r, array);
  }
  void Select(int field_association, int region[4], const char* array = nullptr);
  ///@}

  ///@{
  /**
   * Make a selection with a polygon. The polygon2DArray should contain
   * the polygon points in display units of (x, y) tuples, and arrayLen
   * is the total length of polygon2DArray.
   * This will result in setting up of this->LastSelection
   * which can be accessed using GetLastSelection().
   * \note This method is called on call rendering processes and client (or
   * driver). Thus, if doing client only rendering, this shouldn't be called on
   * server nodes.
   */
  void SelectPolygonPoints(int* polygon2DArray, vtkIdType arrayLen);
  void SelectPolygonCells(int* polygon2DArray, vtkIdType arrayLen);
  void SelectPolygon(int field_association, int* polygon2DArray, vtkIdType arrayLen);
  ///@}

  ///@{
  /**
   * Provides access to the last selection. This is valid only on the client or
   * driver node displaying the composited result.
   */
  vtkGetObjectMacro(LastSelection, vtkSelection);
  ///@}

  ///@{
  /**
   * Set or get whether capture should be done as
   * StillRender or InteractiveRender when capturing screenshots.
   */
  vtkSetMacro(UseInteractiveRenderingForScreenshots, bool);
  vtkBooleanMacro(UseInteractiveRenderingForScreenshots, bool);
  vtkGetMacro(UseInteractiveRenderingForScreenshots, bool);
  ///@}

  ///@{
  /**
   * Returns if remote-rendering is possible on the current group of processes.
   */
  vtkGetMacro(RemoteRenderingAvailable, bool);
  void RemoteRenderingAvailableOff() { this->RemoteRenderingAvailable = false; }
  ///@}

  ///@{
  /**
   * Determine if NVPipe is an available compressor option.
   */
  void NVPipeAvailableOn();
  void NVPipeAvailableOff();
  ///@}

  ///@{
  /**
   * Returns true if the most recent render used LOD.
   */
  vtkGetMacro(UsedLODForLastRender, bool);
  ///@}

  /**
   * Invalidates cached selection. Called explicitly when view proxy thinks the
   * cache may have become obsolete.
   * \note CallOnAllProcesses
   */
  void InvalidateCachedSelection();

  ///@{
  /**
   * Convenience methods used by representations to pass represented data.
   * If trueSize is non-zero, then that's the size used in making decisions
   * about LOD/remote rendering etc and not the actual size of the dataset.
   */
  static vtkAlgorithmOutput* GetPieceProducer(
    vtkInformation* info, vtkPVDataRepresentation* repr, int port = 0);
  static vtkAlgorithmOutput* GetPieceProducerLOD(
    vtkInformation* info, vtkPVDataRepresentation* repr, int port = 0);
  static void SetRedistributionMode(
    vtkInformation* info, vtkPVDataRepresentation* repr, int mode, int port = 0);
  static void SetRedistributionModeToSplitBoundaryCells(
    vtkInformation* info, vtkPVDataRepresentation* repr, int port = 0);
  static void SetRedistributionModeToDuplicateBoundaryCells(
    vtkInformation* info, vtkPVDataRepresentation* repr, int port = 0);
  static void SetRedistributionModeToUniquelyAssignBoundaryCells(
    vtkInformation* info, vtkPVDataRepresentation* repr, int port = 0);
  static void SetGeometryBounds(vtkInformation* info, vtkPVDataRepresentation* repr,
    const double bounds[6], vtkMatrix4x4* transform = nullptr, int port = 0);
  static void SetStreamable(vtkInformation* info, vtkPVDataRepresentation* repr, bool streamable);
  static void SetNextStreamedPiece(
    vtkInformation* info, vtkPVDataRepresentation* repr, vtkDataObject* piece);
  static vtkDataObject* GetCurrentStreamedPiece(
    vtkInformation* info, vtkPVDataRepresentation* repr);
  ///@}

  ///@{
  /**
   * `OrderedCompositingConfiguration` lets representations indicate to the view
   * how the representation participates in data-redistribution necessary when
   * ordered-compositing is being used. These flags are meant to be combined
   * together using bit-wise-OR.
   *
   * If not specified, default is 0 i.e. the representation data will be left
   * untouched. Except for trivial representations or representations that have
   * their full rendering geometry available on all rendering ranks, this is not
   * recommended.
   *
   * `DATA_IS_REDISTRIBUTABLE` indicates that the view may redistribute the
   * rendering data from this representation as needed. This is typical for
   * representation rendering poly-data or unstructured grids.
   *
   * `USE_DATA_FOR_LOAD_BALANCING` indicates that when building a sortable
   * distribution for the data, it should use this representation data. If not
   * set, the representation will not contribute to the algorithm that builds
   * the kd-tree used to split data across rendering ranks.
   *
   * `USE_BOUNDS_FOR_REDISTRIBUTION` may be used when the data is too heavy to move (i.e
   * DATA_IS_REDISTRIBUTABLE is not true) and is already split across ranks in a
   * sortable fashion. In that case, this indicates the view must use the
   * spatial partitioning of this representation's data and move other data
   * accordingly. This flag cannot be combined with DATA_IS_REDISTRIBUTABLE or
   * USE_BOUNDS_FOR_REDISTRIBUTION.
   */
  enum
  {
    USE_BOUNDS_FOR_REDISTRIBUTION = 0x01,
    DATA_IS_REDISTRIBUTABLE = 0x02,
    USE_DATA_FOR_LOAD_BALANCING = 0x40,
  };
  static void SetOrderedCompositingConfiguration(vtkInformation* info,
    vtkPVDataRepresentation* repr, int config, const double* bounds = nullptr, int port = 0);
  ///@}

  /**
   * Used by Cinema to enforce a consistent depth scaling.
   * Called with the global (visible and invisible) bounds at start of export.
   */
  void SetMaxClipBounds(double bds[6]);

  ///@{
  /**
   * Used by Cinema to enforce a consistent viewpoint and depth scaling.
   * Prevents ParaView from changing depth scaling over course of an export.
   */
  void SetLockBounds(bool nv);
  vtkGetMacro(LockBounds, bool);
  ///@}

  /**
   * Requests the view to deliver the pieces produced by the \c repr to all
   * processes after a gather to the root node to merge the datasets generated
   * by each process.
   */
  static void SetDeliverToAllProcesses(
    vtkInformation* info, vtkPVDataRepresentation* repr, bool clone);

  /**
   * Requests the view to deliver the data to the client always. This is
   * essential for representation that render in the non-composited views e.g.
   * the text-source representation. If SetDeliverToAllProcesses() is true, this
   * is redundant. \c gather_before_delivery can be used to indicate if the data
   * on the server-nodes must be gathered to the root node before shipping to
   * the client. If \c gather_before_delivery is false, only the data from the
   * root node will be sent to the client without any parallel communication.
   */
  static void SetDeliverToClientAndRenderingProcesses(vtkInformation* info,
    vtkPVDataRepresentation* repr, bool deliver_to_client, bool gather_before_delivery,
    int port = 0);

  ///@{
  /**
   * Some representation only work when remote rendering or local rendering. Use
   * this method in REQUEST_UPDATE() pass to tell the view if the representation
   * requires a particular mode. Note, only use this to "require" a remote or
   * local render. \c value == true indicates that the representation requires
   * distributed rendering, \c value == false indicates the representation can
   * only render property on the client or root node.
   */
  static void SetRequiresDistributedRendering(
    vtkInformation* info, vtkPVDataRepresentation* repr, bool value, bool for_lod = false);
  static void SetRequiresDistributedRenderingLOD(
    vtkInformation* info, vtkPVDataRepresentation* repr, bool value)
  {
    vtkPVRenderView::SetRequiresDistributedRendering(info, repr, value, true);
  }
  ///@}

  ///@{
  /**
   * This is an temporary/experimental option and may be removed without notice.
   * This is intended to be used within some experimental representations that
   * require that all data being moved around uses a specific mode rather than
   * the one automatically determined based on the process type.
   * Set \c flag to -1 to clear. The flag is cleared in every
   * vtkPVRenderView::Update() call, hence a representation must set it in
   * vtkPVView::REQUEST_UPDATE() pass if needed each time.
   * Also note, if the value it set to non-negative and is not equal to
   * vtkMPIMoveData::PASS_THROUGH,
   * ordered compositing will also be disabled.
   */
  static void SetForceDataDistributionMode(vtkInformation* info, int flag);
  bool IsForceDataDistributionModeSet() const { return this->ForceDataDistributionMode != -1; }
  int GetForceDataDistributionMode() const { return this->ForceDataDistributionMode; }
  ///@}

  ///@{
  /**
   * Representations that support hardware (render-buffer based) selection,
   * should register the prop that they use for selection rendering. They can do
   * that in the vtkPVDataRepresentation::AddToView() implementation.
   */
  int RegisterPropForHardwareSelection(vtkPVDataRepresentation* repr, vtkProp* prop);
  void UnRegisterPropForHardwareSelection(vtkPVDataRepresentation* repr, vtkProp* prop);
  ///@}

  ///@{
  /**
   * Enable/disable showing of annotation for developers.
   */
  void SetShowAnnotation(bool val);
  vtkSetMacro(UpdateAnnotation, bool);
  ///@}

  ///@{
  /**
   * Set color of annotation text for developers
   */
  void SetAnnotationColor(double r, double g, double b);
  ///@}

  /**
   * Set the vtkPVGridAxes3DActor to use for the view.
   */
  virtual void SetGridAxes3DActor(vtkPVGridAxes3DActor*);

  /**
   * Set the vtkLegendScaleActor to use for the view.
   */
  virtual void SetLegendGridActor(vtkLegendScaleActor*);

  /**
   * Set the vtkPolarAxesActor to use for the view.
   */
  virtual void SetPolarGridActor(vtkPolarAxesActor2D*);

  ///@{
  /**
   * Forwarded to orientation axes widget.
   */
  virtual void SetOrientationAxesInteractivity(bool);
  virtual void SetOrientationAxesVisibility(bool);

  void SetOrientationAxesLabelColor(double r, double g, double b);
  void SetOrientationAxesOutlineColor(double r, double g, double b);

  void SetOrientationAxesXColor(double r, double g, double b);
  void SetOrientationAxesYColor(double r, double g, double b);
  void SetOrientationAxesZColor(double r, double g, double b);

  void SetOrientationAxesXVisibility(bool vis);
  void SetOrientationAxesYVisibility(bool vis);
  void SetOrientationAxesZVisibility(bool vis);

  void SetOrientationAxesXLabelText(const char* text);
  void SetOrientationAxesYLabelText(const char* text);
  void SetOrientationAxesZLabelText(const char* text);
  ///@}

  ///@{
  /**
   * Forwarded to camera orientation widget.
   */
  void SetCameraOrientationWidgetVisibility(bool visible);
  void SetCameraOrientationWidgetSize(int size);
  void SetCameraOrientationWidgetPadding(int padding[2]);
  void SetCameraOrientationWidgetAnchor(int anchor);
  ///@}

  /**
   * Forwarded to center axes.
   */
  virtual void SetCenterAxesVisibility(bool);

  ///@{
  /**
   * Forward to vtkPVInteractorStyle instances.
   */
  virtual void SetCenterOfRotation(double x, double y, double z);
  virtual void SetRotationFactor(double factor);
  ///@}

  ///@{
  /**
   * Forward to vtkLightKit.
   */
  void SetKeyLightWarmth(double val);
  void SetKeyLightIntensity(double val);
  void SetKeyLightElevation(double val);
  void SetKeyLightAzimuth(double val);
  void SetFillLightWarmth(double val);
  void SetKeyToFillRatio(double val);
  void SetFillLightElevation(double val);
  void SetFillLightAzimuth(double val);
  void SetBackLightWarmth(double val);
  void SetKeyToBackRatio(double val);
  void SetBackLightElevation(double val);
  void SetBackLightAzimuth(double val);
  void SetHeadLightWarmth(double val);
  void SetKeyToHeadRatio(double val);
  void SetMaintainLuminance(int val);
  ///@}

  ///@{
  /**
   * Forward to 3D renderer.
   */
  vtkSetMacro(UseHiddenLineRemoval, bool);
  virtual void SetUseDepthPeeling(int val);
  virtual void SetUseDepthPeelingForVolumes(bool val);
  virtual void SetMaximumNumberOfPeels(int val);
  virtual void SetBackgroundTexture(vtkTexture* val);
  ///@}

  ///@{
  /**
   * When set, background color and mode will be obtained from
   * vtkPVRenderViewSettings.
   */
  vtkSetMacro(UseRenderViewSettingsForBackground, bool);
  vtkGetMacro(UseRenderViewSettingsForBackground, bool);
  ///@}

  enum BackgroundMode
  {
    DEFAULT,
    GRADIENT,
    IMAGE,
    SKYBOX,
    STEREO_SKYBOX,
  };

  ///@{
  /**
   * API for background color selection.
   *
   * @see BackgroundMode
   */
  vtkSetClampMacro(BackgroundColorMode, int, DEFAULT, STEREO_SKYBOX);
  vtkSetVector3Macro(Background, double);
  vtkGetVector3Macro(Background, double);
  vtkSetVector3Macro(Background2, double);
  vtkGetVector3Macro(Background2, double);
  vtkSetMacro(UseEnvironmentLighting, bool);
  ///@}

  ///@{
  /**
   * Entry point for environmental backgrounds
   */
  virtual void SetEnvironmentalBG(double r, double g, double b);
  virtual void SetEnvironmentalBG2(double r, double g, double b);
  virtual void SetEnvironmentalBGTexture(vtkTexture* val);
  virtual void SetGradientEnvironmentalBG(int val);
  virtual void SetTexturedEnvironmentalBG(int val);
  virtual void SetBackgroundMode(int val);
  ///@}

  ///@{
  /**
   * Entry point for dynamic lights
   */
  void AddLight(vtkLight*);
  void RemoveLight(vtkLight*);
  ///@}

  ///@{
  /**
   * Forward to vtkRenderWindow.
   */
  void SetStereoCapableWindow(int val);
  void SetStereoRender(int val);
  vtkSetMacro(StereoType, int);
  vtkSetMacro(ServerStereoType, int);
  void SetMultiSamples(int val);
  void SetAlphaBitPlanes(int val);
  void SetStencilCapable(int val);
  void SetPhysicalToWorldMatrix(const double[16]);
  ///@}

  /**
   * Forward to vtkCamera.
   */
  void SetParallelProjection(int mode);

  ///@{
  /**
   * Forwarded to vtkPVInteractorStyle if present on local processes.
   */
  virtual void SetCamera2DManipulators(const int manipulators[9]);
  virtual void SetCamera3DManipulators(const int manipulators[9]);
  void SetCameraManipulators(vtkPVInteractorStyle* style, const int manipulators[9]);
  virtual void SetReverseMouseWheelZoomDirection(bool reverse);
  virtual void SetMouseWheelZoomsToCursor(bool value);
  virtual void SetCamera2DMouseWheelMotionFactor(double factor);
  virtual void SetCamera3DMouseWheelMotionFactor(double factor);
  ///@}

  /**
   * Synchronizes bounds information on all nodes and
   * reset camera clipping planes.
   * \note CallOnAllProcesses
   */
  virtual void SynchronizeGeometryBounds();

  /**
   * Overridden to synchronize information among processes whenever data
   * changes. The vtkSMViewProxy ensures that this method is called only when
   * something has changed on the view-proxy or one of its representations or
   * their inputs. Hence it's okay to do some extra inter-process communication
   * here.
   */
  void Update() override;

  /**
   * Asks representations to update their LOD geometries.
   */
  virtual void UpdateLOD();

  ///@{
  /**
   * Returns whether the view will use LOD rendering for the next
   * InteractiveRender() call based on the geometry sizes determined by the most
   * recent call to Update().
   */
  vtkGetMacro(UseLODForInteractiveRender, bool);
  ///@}

  ///@{
  /**
   * Returns whether the view will use distributed rendering for the next
   * full-resolution render. This uses the full resolution geometry sizes as
   * determined by the most recent call to `Update`.
   */
  vtkGetMacro(UseDistributedRenderingForRender, bool);
  ///@}

  ///@{
  /**
   * Returns whether the view will use distributed rendering for the next
   * low-resolution render. This uses the low-resolution (or LOD) geometry sizes
   * as determined by the most recent call to `UpdateLOD`.
   */
  vtkGetMacro(UseDistributedRenderingForLODRender, bool);
  ///@}

  ///@{
  /**
   * Returns the processes (vtkPVSession::ServerFlags) that are to be involved
   * in the next StillRender() call based on the decisions made during the most
   * recent Update().
   */
  vtkGetMacro(StillRenderProcesses, vtkTypeUInt32);
  ///@}

  ///@{
  /**
   * Returns the processes (vtkPVSession::ServerFlags) that are to be involved
   * in the next InteractiveRender() call based on the decisions made during the most
   * recent Update() and UpdateLOD().
   */
  vtkGetMacro(InteractiveRenderProcesses, vtkTypeUInt32);
  ///@}

  /**
   * Returns the data distribution mode to use.
   */
  int GetDataDistributionMode(bool low_res);

  /**
   * Called on all processes to request data-delivery for the list of
   * representations. Note this method has to be called on all processes or it
   * may lead to deadlock.
   */
  void Deliver(int use_lod, unsigned int size, unsigned int* representation_ids) override;

  /**
   * Returns true when ordered compositing is needed on the current group of
   * processes. Note that unlike most other functions, this may return different
   * values on different processes e.g.
   * \li always false on client and dataserver
   * \li true on pvserver or renderserver if opacity < 1 or volume present, else
   * false
   */
  bool GetUseOrderedCompositing();

  /**
   * Returns true when the compositor should not use the empty
   * images optimization.
   */
  bool GetRenderEmptyImages();

  ///@{
  /**
   * Enable/disable FXAA antialiasing.
   */
  vtkSetMacro(UseFXAA, bool);
  vtkGetMacro(UseFXAA, bool);
  ///@}

  ///@{
  /**
   * FXAA tunable parameters. See vtkFXAAOptions for details.
   */
  void SetFXAARelativeContrastThreshold(double val);
  void SetFXAAHardContrastThreshold(double val);
  void SetFXAASubpixelBlendLimit(double val);
  void SetFXAASubpixelContrastThreshold(double val);
  void SetFXAAUseHighQualityEndpoints(bool val);
  void SetFXAAEndpointSearchIterations(int val);
  ///@}

  /**
   * Defines tone mapping generic filmic presets.
   */
  enum GenericFilmicPresets
  {
    Custom = 0,
    Default = 1,
    Uncharted2 = 2
  };

  ///@{
  /**
   * Tone Mapping options.
   */
  void SetUseToneMapping(bool v);
  vtkGetMacro(UseToneMapping, bool);
  void SetToneMappingType(int);
  void SetExposure(double);
  void SetContrast(double);
  void SetShoulder(double);
  void SetMidIn(double);
  void SetMidOut(double);
  void SetHdrMax(double);
  void SetUseACES(bool);
  void SetGenericFilmicPresets(int t);
  ///@}

  ///@{
  /**
   * SSAO options.
   */
  vtkSetMacro(UseSSAO, bool);
  vtkGetMacro(UseSSAO, bool);
  vtkSetMacro(UseSSAODefaultPresets, bool);
  vtkSetMacro(Radius, double);
  vtkSetMacro(KernelSize, int);
  vtkSetMacro(Bias, double);
  vtkSetMacro(Blur, bool);
  ///@}

  /**
   * Copy internal fields that are used for rendering decision such as
   * remote/local rendering, composite and so on. This method was introduced
   * for the quad view so internal views could use the decision that were made
   * in the main view.
   */
  void CopyViewUpdateOptions(vtkPVRenderView* otherView);

  ///@{
  /**
   * Add props directly to the view.
   */
  void AddPropToRenderer(vtkProp* prop);
  void RemovePropFromRenderer(vtkProp* prop);
  ///@}

  ///@{
  /**
   * Experimental API to grab re-colorable images. Between
   * BeginValuePassForRendering and EndValuePassForRendering calls, all renders
   * will end up using vtkValuePass for rendering instead of the standard
   * rendering passes that generate results on screen.
   *
   * GrabValuePassResult must be called between BeginValuePassForRendering and
   * EndValuePassForRendering. Returns the vtkFloatArray grabbed by
   * vtkValuePass.
   *
   * This API is not intended for remote-rendering use-cases. Thus only supported in client-only and
   * pvbatch (or in situ) cases. That's the reason why we are exposing this directly on
   * the vtkPVRenderView rather accessing it via a proxy.
   */
  bool BeginValuePassForRendering(int fieldAssociation, const char* arrayName, int component);
  void EndValuePassForRendering();
  vtkSmartPointer<vtkFloatArray> GrabValuePassResult();
  ///@}

  ///@{
  /**
   * Access to the Z buffer.
   */
  void CaptureZBuffer();
  vtkFloatArray* GetCapturedZBuffer();
  ///@}

  ///@{
  /**
   * Turn on/off synchronizable actors.  This is needed for immersive collaboration,
   * (such as CAVEs) so that various types of vtkActor can be synchronized to all
   * displays.  It is off/disabled by default.
   */
  void SetEnableSynchronizableActors(bool);
  bool GetEnableSynchronizableActors();
  ///@}

  ///@{
  /**
   * Switches between rasterization and ray tracing.
   */
  void SetEnableOSPRay(bool);
  bool GetEnableOSPRay();
  ///@}
  ///@{
  /**
   * Controls whether OSPRay sends casts shadow rays or not.
   */
  void SetShadows(bool);
  bool GetShadows();
  ///@}
  ///@{
  /**
   * Sets the number of occlusion query rays that OSPRay sends at each intersection.
   */
  void SetAmbientOcclusionSamples(int);
  int GetAmbientOcclusionSamples();
  ///@}
  ///@{
  /**
   * Sets the depth of monte carlo samples on the path tracer
   */
  void SetRouletteDepth(int);
  int GetRouletteDepth();
  ///@}

  ///@{
  /**
   * Set the number of primary rays that OSPRay shoots per pixel.
   */
  void SetSamplesPerPixel(int);
  int GetSamplesPerPixel();
  ///@}
  ///@{
  /**
   * Set the number of render passes OSPRay takes to accumulate subsampled color results.
   */
  void SetMaxFrames(int);
  int GetMaxFrames();
  ///@}
  /**
   * Has OSPRay reached the max frames?
   */
  bool GetOSPRayContinueStreaming();
  ///@{
  /**
   * Controls whether to use image denoising to improve appearance.
   */
  void SetDenoise(bool);
  bool GetDenoise();
  ///@}

  ///@{
  /**
   * Dimish or Amplify all lights in the scene.
   */
  void SetLightScale(double);
  double GetLightScale();
  ///@}
  /**
   * Set the OSPRay renderer to use.
   * May be either scivis (default) or pathtracer.
   */
  void SetOSPRayRendererType(std::string);
  ///@{
  /**
   * Control of background orientation for OSPRay.
   */
  void SetBackgroundNorth(double x, double y, double z);
  void SetBackgroundEast(double x, double y, double z);
  ///@}
  /**
   * For OSPRay, set the library of materials.
   */
  virtual void SetMaterialLibrary(vtkPVMaterialLibrary*);
  void SetViewTime(double value) override;
  ///@{
  /**
   * Set the size of OSPRay's temporal cache.
   */
  void SetTimeCacheSize(int);
  int GetTimeCacheSize();
  ///@}

  ///@{
  /**
   * DiscreteCameras are a collection of cameras when specified,
   * forces the view to only interact *to* a camera in the collection.
   *
   * In `vtkPVView::REQUEST_UPDATE()` pass, representations may request the view
   * to use discrete cameras by providing a vtkPVCameraCollection to the view. Since
   * multiple representations may be visible in the view, it's up to the
   * representations how to handle multiple representations providing different
   * styles.
   *
   * When set, on each render, vtkPVRenderView will try to update the current
   * camera to match a camera in the collection. During interacting, however,
   * the snapping to a camera in the collection is only done when the snapped to
   * camera is different from the previous. This avoids side effects on
   * camera manipulators that simply update existing camera positions during
   * interaction.
   *
   * @note Since this is supposed to set in vtkPVView::REQUEST_UPDATE(), it is unset
   * before the pass is triggered.
   *
   * @warning This is a new/experimental feature that was added to support
   * viewing of Cinema databases in ParaView. As the support for Cinema in
   * ParaView improve, this is likely to change.
   */
  static vtkPVCameraCollection* GetDiscreteCameras(
    vtkInformation* info, vtkPVDataRepresentation* repr);
  static void SetDiscreteCameras(
    vtkInformation* info, vtkPVDataRepresentation* repr, vtkPVCameraCollection* style);
  ///@}

  // Get the RenderViewBase used by this
  vtkGetObjectMacro(RenderView, vtkRenderViewBase);

  /**
   * Overridden to scale the OrientationWidget appropriately.
   */
  void ScaleRendererViewports(const double viewport[4]) override;

  /**
   * This is used by vtkPVHardwareSelector to synchronize element ids between
   * all ranks involved in selection.
   */
  void SynchronizeMaximumIds(vtkIdType* maxPointId, vtkIdType* maxCellId);

  /**
   *
   */
  void SetHardwareSelector(vtkPVHardwareSelector* selector);

protected:
  vtkPVRenderView();
  ~vtkPVRenderView() override;

  /**
   * Actual render method.
   */
  virtual void Render(bool interactive, bool skip_rendering);

  /**
   * Called  just before the local process renders. This is only called on the
   * nodes where the rendering is going to happen.
   */
  virtual void AboutToRenderOnLocalProcess(bool interactive) { (void)interactive; }

  /**
   * Returns true if distributed rendering should be used based on the geometry
   * size. \c using_lod will be true if this method is called to determine
   * distributed rendering status for renders using lower LOD i.e when called in
   * UpdateLOD().
   */
  bool ShouldUseDistributedRendering(double geometry_size, bool using_lod);

  /**
   * Returns true if LOD rendering should be used based on the geometry size.
   */
  bool ShouldUseLODRendering(double geometry);

  /**
   * Returns true if the local process is invovled in rendering composited
   * geometry i.e. geometry rendered in view that is composited together.
   */
  bool IsProcessRenderingGeometriesForCompositing(bool using_distributed_rendering);

  /**
   * Set the last selection object.
   */
  void SetLastSelection(vtkSelection*);

  /**
   * UpdateCenterAxes().
   * Updates CenterAxes's scale and position.
   */
  virtual void UpdateCenterAxes();

  /**
   * Returns true if the local process is doing to do actual render or
   * displaying an image in a viewport.
   */
  bool GetLocalProcessDoesRendering(bool using_distributed_rendering);

  /**
   * In multi-clients mode, ensures that all processes are in the same "state"
   * as far as the view is concerned. Returns false if that's not the case.
   */
  bool TestCollaborationCounter();

  /**
   * Synchronizes remote-rendering related parameters for collaborative
   * rendering in multi-clients mode.
   */
  void SynchronizeForCollaboration();

  /**
   * Method to build annotation text to annotate the view with runtime
   * information.
   */
  virtual void BuildAnnotationText(ostream& str);

  ///@{
  /**
   * SynchronizationCounter is used in multi-clients mode to ensure that the
   * views on two different clients are in the same state as the server side.
   */
  vtkGetMacro(SynchronizationCounter, unsigned int);
  ///@}

  ///@{
  /**
   * Returns true is currently generating a selection.
   */
  vtkGetMacro(MakingSelection, bool);
  ///@}

  /**
   * Prepare for selection.
   * Returns false if it is currently generating a selection.
   */
  virtual bool PrepareSelect(int fieldAssociation, const char* array = nullptr);

  /**
   * Post process after selection.
   */
  virtual void PostSelect(vtkSelection* sel, const char* array = nullptr);

  /**
   * Updates background color. If no renderer is specified, then the default
   * renderer returned by `GetRenderer` is used.
   */
  virtual void UpdateBackground(vtkRenderer* renderer = nullptr);

  /**
   * Configure texture based on scalar type
   */
  void ConfigureTexture(vtkTexture* texture);

  /**
   * Set the 3D renderer to use within the render view.
   * This function modifies the input renderer in order
   * to be usable by the render view.
   */
  virtual void SetupAndSetRenderer(vtkRenderer* ren);

  vtkLightKit* LightKit;
  vtkRenderViewBase* RenderView;
  vtkRenderer* NonCompositedRenderer;
  vtkPVSynchronizedRenderer* SynchronizedRenderers;
  vtkSmartPointer<vtkRenderWindowInteractor> Interactor;
  vtkInteractorStyleRubberBand3D* RubberBandStyle;
  vtkInteractorStyleRubberBandZoom* RubberBandZoom;
  vtkInteractorStyleDrawPolygon* PolygonStyle;
  vtkPVCenterAxesActor* CenterAxes;
  vtkPVAxesWidget* OrientationWidget;
  vtkSmartPointer<vtkPVHardwareSelector> Selector;
  vtkSelection* LastSelection;
  vtkSmartPointer<vtkPVGridAxes3DActor> GridAxes3DActor;
  vtkSmartPointer<vtkLegendScaleActor> LegendGridActor;
  vtkSmartPointer<vtkPolarAxesActor2D> PolarAxesActor;
  vtkNew<vtkSkybox> Skybox;
  vtkNew<vtkCameraOrientationWidget> CameraOrientationWidget;

  int StillRenderImageReductionFactor;
  int InteractiveRenderImageReductionFactor;
  int InteractionMode;
  bool ShowAnnotation;
  bool UpdateAnnotation;

  // this ivar can be used to suppress the render within
  // a StillRender or InteractiveRender. This is useful
  // in cases where you want the representations mappers
  // to be setup for rendering and have their data ready
  // but not actually do the render. For example if you
  // want to export the scene but not render it you must
  // turn on SuppressRendering and then call StillRender
  bool SuppressRendering;

  // 2D and 3D interactor style
  vtkPVInteractorStyle* TwoDInteractorStyle;
  vtkPVInteractorStyle* ThreeDInteractorStyle;

  bool ReverseMouseWheelZoomDirection = false;

  // Active interactor style either [TwoDInteractorStyle, ThreeDInteractorStyle]
  vtkPVInteractorStyle* InteractorStyle;

  vtkWeakPointer<vtkPVCameraCollection> DiscreteCameras;

  // Used in collaboration mode to ensure that views are in the same state
  // (as far as representations added/removed goes) before rendering.
  unsigned int SynchronizationCounter;

  // In mega-bytes.
  double RemoteRenderingThreshold;
  double LODRenderingThreshold;
  vtkBoundingBox GeometryBounds;

  bool UseInteractiveRenderingForScreenshots;
  bool NeedsOrderedCompositing;
  bool RenderEmptyImages;

  bool UseFXAA;
  vtkNew<vtkFXAAOptions> FXAAOptions;

  bool UseToneMapping;

  bool UseSSAO;
  bool UseSSAODefaultPresets;
  double Radius;
  int KernelSize;
  double Bias;
  bool Blur;

  double LODResolution;
  bool UseLightKit;

  bool UsedLODForLastRender;
  bool UseLODForInteractiveRender;
  bool UseOutlineForLODRendering;
  bool UseDistributedRenderingForRender;
  bool UseDistributedRenderingForLODRender;

  vtkTypeUInt32 StillRenderProcesses;
  vtkTypeUInt32 InteractiveRenderProcesses;

  /**
   * Keeps track of the time when the priority-queue for streaming was
   * generated.
   */
  vtkTimeStamp PriorityQueueBuildTimeStamp;

  bool LockBounds;

private:
  vtkPVRenderView(const vtkPVRenderView&) = delete;
  void operator=(const vtkPVRenderView&) = delete;

  bool MakingSelection;
  int PreviousSwapBuffers;
  void OnSelectionChangedEvent();
  void OnPolygonSelectionEvent();
  void FinishSelection(vtkSelection*, const char*);

  ///@{
  /**
   * Add/remove the given prop in the view.
   * Update culler list accordingly.
   */
  void AddAnnotationToView(vtkProp*);
  void RemoveAnnotationFromView(vtkProp*);
  ///@}

  // This flag is set to false when not all processes cannot render e.g. cannot
  // open the DISPLAY etc.
  bool RemoteRenderingAvailable;

  // Flags used to maintain rendering modes requested by representations.
  bool DistributedRenderingRequired;
  bool NonDistributedRenderingRequired;
  bool DistributedRenderingRequiredLOD;
  bool NonDistributedRenderingRequiredLOD;

  // Cached value for parallel projection set on camera.
  int ParallelProjection;

  // Cached state. Is currently ignored for distributed rendering.
  bool UseHiddenLineRemoval;

  class vtkInternals;
  vtkInternals* Internals;

  vtkNew<vtkTextRepresentation> Annotation;
  void UpdateAnnotationText();

  vtkNew<vtkOrderedCompositingHelper> OrderedCompositingHelper;

  int StereoType;
  int ServerStereoType;
  void UpdateStereoProperties();

  int BackgroundColorMode;
  bool UseEnvironmentLighting;
  bool UseRenderViewSettingsForBackground;
  double Background[3];
  double Background2[3];

  vtkSmartPointer<vtkCuller> Culler;
  vtkNew<vtkTimerLog> Timer;

  int ForceDataDistributionMode;
  int PreviousDiscreteCameraIndex;
  vtkSmartPointer<vtkTexture> EnvironmentalBGTexture;
  bool UseTexturedEnvironmentalBG;
};

#endif

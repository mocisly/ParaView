r"""
Internal module used by paraview.servermanager to help warn about
proxies/properties changed or removed. To help
with backward compatibility, the _backwardscompatibilityhelper module
should be updated to handle deprecated proxies and properties.

Each proxy is associated to a kind of python class. When a proxy is removed,
this class is no more generated and older scripts will raise `NameError`
exception. To avoid this, deprecated proxies should be added in `get_deprecated_proxies`
returned map, so a fallback proxy can be used.
See also `GetProxy`.

Properties are defined as attribute of the python object. When a property
is removed, old scripts using it will fail with `AttributeError`.
When everything else fails, `NotSupportedException` is thrown.
See `setattr` and `getattr` methods.

Each compatibility code is called depending on the current version
and on the compatibility version asked by the script.
Compatibility version should be specified before importing `simple` module.
For instance:

.. code:: python

    import paraview
    paraview.compatibility.major=5
    paraview.compatibility.minor=11
    from paraview.simple import *

Will create fallback proxies and properties for deprecation done since 5.11
and will fail for object deprecated before 5.11.
"""

import paraview
import builtins
from . import servermanager as sm

NotSupportedException = paraview.NotSupportedException


class _Cache:
    LastCompatibilityVersion = None


def get_paraview_compatibility_version():
    """
    Returns the compatibility version set by the user. If the compatibility
    version is set, it also checks if the compatibility version is greater or
    less than the current version and prints a warning.
    """
    compatibility_version = paraview.compatibility.GetVersion()
    current_version = (sm.vtkSMProxyManager.GetVersionMajor(), sm.vtkSMProxyManager.GetVersionMinor())
    # Check if compatibility version is set.
    if compatibility_version == ():
        return current_version
    # Check if compatibility version is greater or less than current version.
    if compatibility_version < current_version and (
            _Cache.LastCompatibilityVersion is None or _Cache.LastCompatibilityVersion != compatibility_version):
        paraview.print_warning(
            "Compatibility version is set to %s while current version is %s. Executing with backwards compatibility "
            "mode enabled." % (compatibility_version, current_version))
        _Cache.LastCompatibilityVersion = compatibility_version
    elif compatibility_version > current_version and (
            _Cache.LastCompatibilityVersion is None or _Cache.LastCompatibilityVersion != compatibility_version):
        paraview.print_warning(
            "Compatibility version is set to %s while current version is %s. Use it at your own risk."
            % (compatibility_version, current_version))
    return compatibility_version


class Continue(Exception):
    pass


class _CubeAxesHelper(object):
    def __init__(self):
        self.CubeAxesVisibility = 0
        self.CubeAxesColor = [0, 0, 0]
        self.CubeAxesCornerOffset = 0.0
        self.CubeAxesFlyMode = 1
        self.CubeAxesInertia = 1
        self.CubeAxesTickLocation = 0
        self.CubeAxesXAxisMinorTickVisibility = 0
        self.CubeAxesXAxisTickVisibility = 0
        self.CubeAxesXAxisVisibility = 0
        self.CubeAxesXGridLines = 0
        self.CubeAxesXTitle = ""
        self.CubeAxesUseDefaultXTitle = 0
        self.CubeAxesYAxisMinorTickVisibility = 0
        self.CubeAxesYAxisTickVisibility = 0
        self.CubeAxesYAxisVisibility = 0
        self.CubeAxesYGridLines = 0
        self.CubeAxesYTitle = ""
        self.CubeAxesUseDefaultYTitle = 0
        self.CubeAxesZAxisMinorTickVisibility = 0
        self.CubeAxesZAxisTickVisibility = 0
        self.CubeAxesZAxisVisibility = 0
        self.CubeAxesZGridLines = 0
        self.CubeAxesZTitle = ""
        self.CubeAxesUseDefaultZTitle = 0
        self.CubeAxesGridLineLocation = 0
        self.DataBounds = [0, 1, 0, 1, 0, 1]
        self.CustomBounds = [0, 1, 0, 1, 0, 1]
        self.CustomBoundsActive = 0
        self.OriginalBoundsRangeActive = 0
        self.CustomRange = [0, 1, 0, 1, 0, 1]
        self.CustomRangeActive = 0
        self.UseAxesOrigin = 0
        self.AxesOrigin = [0, 0, 0]
        self.CubeAxesXLabelFormat = ""
        self.CubeAxesYLabelFormat = ""
        self.CubeAxesZLabelFormat = ""
        self.StickyAxes = 0
        self.CenterStickyAxes = 0


_ACubeAxesHelper = _CubeAxesHelper()


def setattr(proxy, pname, value):
    """
    Attempts to emulate setattr() when called using a deprecated name for a
    proxy property.

    For properties that are no longer present on a proxy, the code should do the
    following:

    1. If `compatibility_version` is less than the version in
       which the property was removed, attempt to handle the request and raise
       `Continue` to indicate that the request has been handled. If it is too
       complicated to support the old API, then it is acceptable to raise
       a warning message, but don't raise an exception.

    2. If compatibility version is >= the version in which the property was
       removed, raise `NotSupportedException` with details including suggestions
       to update the script.
    """
    compatibility_version = get_paraview_compatibility_version()

    if pname == "ColorAttributeType" and proxy.SMProxy.GetProperty("ColorArrayName"):
        if compatibility_version <= (4, 1):
            # set ColorAttributeType on ColorArrayName property instead.
            caProp = proxy.GetProperty("ColorArrayName")
            proxy.GetProperty("ColorArrayName").SetData((value, caProp[1]))
            raise Continue()
        else:
            # if ColorAttributeType is being used, raise NotSupportedException.
            raise NotSupportedException(
                "'ColorAttributeType' is obsolete as of ParaView 4.2. Simply use 'ColorArrayName' "
                "instead. Refer to ParaView Python API changes documentation online.")

    if pname == "AspectRatio" and proxy.SMProxy.GetProperty("ScalarBarThickness"):
        if compatibility_version <= (5, 3):
            # We can't do this perfectly, so we set the ScalarBarThickness
            # property instead. Assume a reasonable modern screen size of
            # 1280x1024 with a Render view size of 1000x600. Even if we had
            # access to the View proxy to get the screen size, there is no
            # guarantee that the view size will remain the same later on in the
            # Python script.
            span = 600  # vertical
            if proxy.GetProperty("Orientation").GetData() == "Horizontal":
                span = 1000

            # Assume a scalar bar length 40% of the span.
            thickness = 0.4 * span / value

            proxy.GetProperty("ScalarBarThickness").SetData(int(thickness))
            raise Continue()
        else:
            # if AspectRatio is being used, raise NotSupportedException
            raise NotSupportedException(
                "'AspectRatio' is obsolete as of ParaView 5.4. Use the "
                "'ScalarBarThickness' property to set the width instead.")

    if pname == "Position2" and proxy.SMProxy.GetProperty("ScalarBarLength"):
        if compatibility_version <= (5, 3):
            # The scalar bar length corresponds to Position2[0] when the
            # orientation is horizontal and Position2[1] when the orientation
            # is vertical.
            length = value[0]
            if proxy.Orientation == "Vertical":
                length = value[1]

            proxy.GetProperty("ScalarBarLength").SetData(length)
        else:
            # if Position2 is being used, raise NotSupportedException
            raise NotSupportedException(
                "'Position2' is obsolete as of ParaView 5.4. Use the "
                "'ScalarBarLength' property to set the length instead.")

    if pname == "LockScalarRange" and proxy.SMProxy.GetProperty("AutomaticRescaleRangeMode"):
        if compatibility_version <= (5, 4):
            from paraview.modules.vtkRemotingViews import vtkSMTransferFunctionManager
            if value:
                proxy.GetProperty("AutomaticRescaleRangeMode").SetData(vtkSMTransferFunctionManager.NEVER)
            else:
                proxy.GetProperty("AutomaticRescaleRangeMode").SetData(vtkSMTransferFunctionManager.GROW_ON_APPLY)

            raise Continue()
        else:
            raise NotSupportedException(
                "'LockScalarRange' is obsolete as of ParaView 5.5. Use "
                "'AutomaticRescaleRangeMode' property instead.")

    # In 5.5, we changed the vtkArrayCalculator to use a different set of constants to control which
    # data it operates on.  This change changed the method and property name from AttributeMode to
    # AttributeType
    if pname == "AttributeMode" and proxy.SMProxy.GetXMLName() == "Calculator":
        if compatibility_version <= (5, 4):
            # The Attribute type uses enumeration values from vtkDataObject::AttributeTypes
            # rather than custom constants for the calculator.  For the values supported by
            # ParaView before this change, the conversion works out to subtracting 1 if value
            # is an integer. If value is an enumerated string we use that as is since it matches
            # the previous enumerated string options.
            if isinstance(value, int):
                proxy.GetProperty("AttributeType").SetData(value - 1)
            else:
                proxy.GetProperty("AttributeType").SetData(value)
            raise Continue()
        else:
            raise NotSupportedException(
                "'AttributeMode' is obsolete.  Use 'AttributeType' property of Calculator filter instead.")

    if pname == "UseOffscreenRenderingForScreenshots" and proxy.SMProxy.IsA("vtkSMViewProxy"):
        if compatibility_version <= (5, 4):
            raise Continue()
        else:
            raise NotSupportedException(
                "'UseOffscreenRenderingForScreenshots' is obsolete. Simply remove it from your script.")
    if pname == "UseOffscreenRendering" and proxy.SMProxy.IsA("vtkSMViewProxy"):
        if compatibility_version <= (5, 4):
            raise Continue()
        else:
            raise NotSupportedException(
                "'UseOffscreenRendering' is obsolete. Simply remove it from your script.")

    if proxy.SMProxy and proxy.SMProxy.GetXMLName() == "CGNSSeriesReader":
        # in 5.5, CGNS reader had some changes. "BaseStatus", "FamilyStatus",
        # "LoadBndPatch", and "LoadMesh" properties were removed.
        if pname in ["LoadBndPatch", "LoadMesh", "BaseStatus", "FamilyStatus"]:
            if compatibility_version <= (5, 4):
                paraview.print_warning(
                    "'%s' is no longer supported and will have no effect. "
                    "Please use `Blocks` property to specify blocks to load." % pname)
                raise Continue()
            else:
                raise NotSupportedException("'%s' is obsolete. Use the `Blocks` "
                                            "property to select blocks using SIL instead." % pname)

    if pname == "DataBoundsInflateFactor" and proxy.SMProxy.GetProperty("DataBoundsScaleFactor"):
        if compatibility_version <= (5, 4):
            # In 5.5, The axes grid data bounds inflate factor have been
            # translated by 1 to become the scale factor.
            proxy.GetProperty("DataBoundsScaleFactor").SetData(value + 1)
        else:
            # if inflat factor is being used, raise NotSupportedException
            raise NotSupportedException(
                "'DataBoundsInflateFactor' is obsolete as of ParaView 5.5. Use the ""'DataBoundsScaleFactor' property "
                "to modify the axes gris data bounds instead.")

    if proxy.SMProxy and proxy.SMProxy.GetXMLName() == "AnnotateAttributeData":
        # in 5.5, Annotate Attribute Data changed how it sets the array to annotate
        if pname == "ArrayAssociation":
            if compatibility_version <= (5, 4):
                paraview.print_warning(
                    "'ArrayAssociation' is obsolete.  Use 'SelectInputArray' property of AnnotateAttributeData instead."
                )

                if value == "Point Data":
                    value = "POINTS"
                elif value == "Cell Data":
                    value = "CELLS"
                elif value == "Field Data":
                    value = "FIELD"
                elif value == "Row Data":
                    value = "ROWS"

                arrayProp = proxy.GetProperty("SelectInputArray")
                proxy.GetProperty("SelectInputArray").SetData((value, arrayProp[1]))
                raise Continue()
            else:
                raise NotSupportedException(
                    "'ArrayAssociation' is obsolete as of ParaView 5.5.  Use 'SelectInputArray' instead.")
        elif pname == "ArrayName":
            if compatibility_version <= (5, 4):
                paraview.print_warning(
                    "'ArrayName' is obsolete.  Use 'SelectInputArray' property of AnnotateAttributeData instead.")

                arrayProp = proxy.GetProperty("SelectInputArray")
                proxy.GetProperty("SelectInputArray").SetData((arrayProp[0], value))
                raise Continue()
            else:
                raise NotSupportedException(
                    "'ArrayName' is obsolete as of ParaView 5.5.  Use 'SelectInputArray' instead.")

    # In 5.5, we changed the Clip to be inverted from what it was before and changed the InsideOut
    # property to be called Invert to be clearer.
    if pname == "InsideOut" and proxy.SMProxy.GetXMLName() == "Clip":
        if compatibility_version <= (5, 4):
            proxy.GetProperty("Invert").SetData(1 - value)
            raise Continue()
        else:
            raise NotSupportedException(
                "'InsideOut' is obsolete.  Use 'Invert' property of Clip filter instead.")

    # In 5.6, we changed the "SpreadSheetRepresentation" proxy to no longer have
    # the "FieldAssociation" and "GenerateCellConnectivity" properties. They are
    # now moved to the view.
    if pname in ["FieldAssociation",
                 "GenerateCellConnectivity"] and proxy.SMProxy.GetXMLName() == "SpreadSheetRepresentation":
        if compatibility_version <= (5, 5):
            raise Continue()
        else:
            raise NotSupportedException(
                "'%s' is obsolete on SpreadSheetRepresentation as of ParaView 5.6 and has been migrated to the view."
                % pname)

    # In 5.7, we changed to the names of the input proxies in ResampleWithDataset to clarify what
    # each source does.
    if pname == "Input" and proxy.SMProxy.GetXMLName() == "ResampleWithDataset":
        if compatibility_version < (5, 7):
            proxy.GetProperty("SourceDataArrays").SetData(value)
            raise Continue()
        else:
            raise NotSupportedException(
                'The ResampleWithDataset.Input property has been changed in ParaView 5.7. '
                'Please set the SourceDataArrays property instead.')

    if pname == "Source" and proxy.SMProxy.GetXMLName() == "ResampleWithDataset":
        if compatibility_version < (5, 7):
            proxy.GetProperty("DestinationMesh").SetData(value)
            raise Continue()
        else:
            raise NotSupportedException(
                'The ResampleWithDataset.Source property has been changed in ParaView 5.7. '
                'Please set the DestinationMesh property instead.')

    # In 5.7, we removed `ArrayName` property on the `GenerateIdScalars` filter
    # and replaced it with `CellIdsArrayName` and `PointIdsArrayName`. Filter was also renamed
    # in "PointAndCellIds" in 5.13
    if pname == "ArrayName" and proxy.SMProxy.GetXMLName() in ("GenerateIdScalars", "PointAndCellIds"):
        if compatibility_version < (5, 7):
            proxy.GetProperty("PointIdsArrayName").SetData(value)
            proxy.GetProperty("CellIdsArrayName").SetData(value)
            raise Continue()
        else:
            raise NotSupportedException(
                'The GenerateIdScalars.ArrayName property has been removed in ParaView 5.7. '
                'Please set `PointIdsArrayName` or `CellIdsArrayName` property instead.')

    # In 5.7, we renamed the 3D View's ray tracing interface from OSPRay to RayTracing
    if pname == "EnableOSPRay" and proxy.SMProxy.IsA("vtkSMRenderViewProxy"):
        if compatibility_version < (5, 7):
            return proxy.GetProperty("EnableRayTracing").SetData(value)
        else:
            raise NotSupportedException(
                'The `EnableOSPRay` control has been renamed in ParaView 5.7 to `EnableRayTracing`.')
    if pname == "OSPRayRenderer" and proxy.SMProxy.IsA("vtkSMRenderViewProxy"):
        newvalue = "OSPRay raycaster"
        if value == "pathtracer":
            newvalue = "OSPRay pathtracer"
        if compatibility_version < (5, 7):
            return proxy.GetProperty("BackEnd").SetData(newvalue)
        else:
            raise NotSupportedException(
                'The `OSPRayRenderer` control has been renamed in ParaView 5.7 to `BackEnd` and '
                'the settings `scivis` and `pathtracer` have been renamed to `OSPRay scivis` '
                'and `OSPRay pathtracer` respectively.')
    if pname == "OSPRayTemporalCacheSize" and proxy.SMProxy.IsA("vtkSMRenderViewProxy"):
        if compatibility_version < (5, 7):
            return proxy.GetProperty("TemporalCacheSize").SetData(value)
        else:
            raise NotSupportedException(
                'The `OSPRayTemporalCacheSize` control has been renamed in ParaView 5.7 to `TemporalCacheSize`.')
    if pname == "OSPRayUseScaleArray" and proxy.SMProxy.IsA("vtkSMRepresentationProxy"):
        if compatibility_version < (5, 7):
            return proxy.GetProperty("UseScaleArray").SetData(value)
        else:
            raise NotSupportedException(
                'The `OSPRayUseScaleArray` control has been renamed in ParaView 5.7 to `UseScaleArray`.')
    if pname == "OSPRayScaleFunction" and proxy.SMProxy.IsA("vtkSMRepresentationProxy"):
        if compatibility_version < (5, 7):
            return proxy.GetProperty("ScaleFunction").SetData(value)
        else:
            raise NotSupportedException(
                'The `OSPRayScaleFunction` control has been renamed in ParaView 5.7 to `ScaleFunction`.')
    if pname == "OSPRayMaterial" and proxy.SMProxy.IsA("vtkSMRepresentationProxy"):
        if compatibility_version < (5, 7):
            return proxy.GetProperty("Material").SetData(value)
        else:
            raise NotSupportedException(
                'The `OSPRayMaterial` control has been renamed in ParaView 5.7 to `Material`.')

    if pname == "CompositeDataSetIndex" and proxy.SMProxy.GetXMLName() == "SpreadSheetRepresentation":
        if compatibility_version <= (5, 9):
            selectors = ["//*[@cid='%s']" % cid for cid in value]
            return proxy.GetProperty("BlockVisibilities").SetData(selectors)
        else:
            raise NotSupportedException(
                "SpreadSheetRepresentation no longer uses 'CompositeDataSetIndex' but instead "
                "supports 'Selectors' to select blocks.")

    if pname == "BlockIndices" and proxy.SMProxy.GetXMLName() == "ExtractBlock":
        if compatibility_version <= (5, 9):
            selectors = ["//*[@cid='%s']" % cid for cid in value]
            return proxy.GetProperty("Selectors").SetData(selectors)
        else:
            raise NotSupportedException(
                "ExtractBlock no longer uses 'BlockIndices' but instead "
                "supports 'Selectors' to select blocks.")

    if pname in ["MaintainStructure", "PruneOutput"] and proxy.SMProxy.GetXMLName() == "ExtractBlock":
        if compatibility_version <= (5, 9):
            return
        else:
            raise NotSupportedException(
                "ExtractBlock no longer supported '%s'. Simply remove it." % pname)

    if pname in ["UseGradientBackground", "UseTexturedBackground",
                 "UseSkyboxBackground"] and proxy.SMProxy.GetXMLGroup() == "views" \
            and proxy.SMProxy.GetProperty("BackgroundColorMode"):
        if compatibility_version <= (5, 9):
            mode = proxy.GetProperty("BackgroundColorMode").GetData()
            if pname == "UseGradientBackground":
                if value == 1 and mode != "Gradient":
                    proxy.BackgroundColorMode = "Gradient"
                elif value == 0 and mode == "Gradient":
                    proxy.BackgroundColorMode = "Single Color"
            elif pname == "UseTexturedBackground":
                if value == 1 and mode != "Texture":
                    proxy.BackgroundColorMode = "Texture"
                elif value == 0 and mode == "Texture":
                    proxy.BackgroundColorMode = "Single Color"
            elif pname == "UseSkyboxBackground":
                if value == 1 and mode != "Skybox":
                    proxy.BackgroundColorMode = "Skybox"
                elif value == 0 and mode == "Skybox":
                    proxy.BackgroundColorMode = "Single Color"
            raise Continue()
        else:
            raise NotSupportedException("'%s' is no longer supported. Use "
                                        "'BackgroundColorMode' instead to choose background color mode." % pname)

    if pname == "ThresholdRange" and proxy.SMProxy.GetXMLName() == "Threshold":
        # From ParaView 5.10, the Threshold filter now offers additional
        # thresholding methods besides ThresholdBetween. The lower and upper
        # threshold values are also set separately.
        if compatibility_version <= (5, 9):
            proxy.GetProperty("ThresholdMethod").SetData(0)
            proxy.GetProperty("LowerThreshold").SetData(value[0])
            proxy.GetProperty("UpperThreshold").SetData(value[1])
            raise Continue()
        else:
            raise NotSupportedException("The 'ThresholdRange' property has been "
                                        "removed in ParaView 5.10. Please set the lower and upper "
                                        "thresholds using the 'LowerThreshold' and 'UpperThreshold' "
                                        "properties, then set the 'ThresholdMethod' property to "
                                        "'Between' to threshold between the lower and upper thresholds.")

    # In 5.11, we changed the ParticleTracer/ParticlePath/StreakLine's the StaticMesh
    # property to be called MeshOverTime since more capabilities were added.
    if pname == "StaticMesh" and proxy.SMProxy.GetXMLName() in ["ParticleTracer, ParticlePath, StreakLine"]:
        if compatibility_version <= (5, 10):
            proxy.GetProperty("MeshOverTime").SetData(value)
            raise Continue()
        else:
            raise NotSupportedException("'StaticMesh' is obsolete.  Use 'MeshOverTime' property of " +
                                        proxy.SMProxy.GetXMLName() + " filter instead.")

    # 5.11 -> 5.12 breaking changes on "TableFFT" properties
    # Renamed AverageFFTperblock into UseWelchMethod
    # Renamed OptimizeForRealInput into OneSidedSpectrum
    # Removed NumberOfBlock
    if proxy.SMProxy and proxy.SMProxy.GetXMLName() == "TableFFT":
        if pname == "AverageFFTperblock":
            if compatibility_version < (5, 12):
                proxy.GetProperty("UseWelchMethod").SetData(value)
                raise Continue()
            else:
                raise NotSupportedException("'AverageFFTperblock' is obsolete.  Use 'UseWelchMethod' property instead.")
        elif pname == "OptimizeForRealInput":
            if compatibility_version < (5, 12):
                proxy.GetProperty("OneSidedSpectrum").SetData(value)
                raise Continue()
            else:
                raise NotSupportedException(
                    "'OptimizeForRealInput' is obsolete.  Use 'OneSidedSpectrum' property instead.")
        elif pname == "NumberOfBlock" and not compatibility_version < (5, 12):
            raise NotSupportedException("'NumberOfBlock' is obsolete.  See 'BlockOverlap' property instead.")

    # 5.11 -> 5.12 we replaced MergePoints with Locator for Cut
    # Need to specify which locator use instead of toggling
    if proxy.SMProxy and proxy.SMProxy.GetXMLName() == "Cut":
        if pname == "MergePoints":
            if compatibility_version < (5, 12):
                if value:
                    proxy.GetProperty("Locator").SetData("UniformBinning")
                else:
                    proxy.GetProperty("Locator").SetData("NotMergingPoints")
                raise Continue()
            else:
                raise NotSupportedException("'MergePoints' is obsolete.  Use 'Locator' property instead.")

    # 5.12 -> 5.13
    # breaking change on Representation properties
    # Renamed Position into Translation
    if proxy.SMProxy and proxy.SMProxy.GetXMLName().endswith("Representation"):
        if pname == "Position":
            if compatibility_version < (5, 13):
                proxy.GetProperty("Translation").SetData(value)
                raise Continue()
            else:
                raise NotSupportedException("'Position' is obsolete.  Use 'Translation' property instead.")
    # FlipTextures has been deprecated in favor of TextureTransform
    if pname == "FlipTextures" and proxy.SMProxy.IsA("vtkSMRepresentationProxy"):
        if compatibility_version < (5, 13):
            scale = [1, -1 if value else 1, 1]
            proxy.GetProperty("TextureTransform").GetProxy(0).GetProperty("Scale").SetElements(scale)
            raise Continue()
        else:
            raise NotSupportedException(
                "'FlipTextures' is obsolete.  Use 'TextureTransform' property of representation instead.")

    # 5.13 -> 6.0 HyperTreeGridAxisReflection replaced by AxisAlignedReflect
    # PlaneNormal and PlanePosition have been replaced by a vtkPlane 'ReflectionPlane'
    if proxy.SMProxy and proxy.SMProxy.GetXMLName() in ("HyperTreeGridAxisReflection", "AxisAlignedReflect"):
        if pname == "PlaneNormal":
            if compatibility_version < (6, 0):
                if type(value).__name__ == 'str':
                    if value == 'X Axis':
                        value = 6
                    elif value == 'Y Axis':
                        value = 7
                    elif value == 'Z Axis':
                        value = 8
                if value == 6:
                    proxy.GetProperty("ReflectionPlane").GetData().Normal = [1, 0, 0]
                elif value == 7:
                    proxy.GetProperty("ReflectionPlane").GetData().Normal = [0, 1, 0]
                elif value == 8:
                    proxy.GetProperty("ReflectionPlane").GetData().Normal = [0, 0, 1]
                raise Continue()
            else:
                raise NotSupportedException("'PlaneNormal' property has been removed in ParaView 6.0. Please use ReflectionPlane to define the plane instead.")
        if pname == "PlanePosition":
            if compatibility_version < (6, 0):
                proxy.GetProperty("ReflectionPlane").GetData().Origin = [value, value, value]
            else:
                raise NotSupportedException("'PlanePosition' property has been removed in ParaView 6.0. Please use ReflectionPlane to define the plane instead.")

    # 5.13 -> 6.0 Reflect replaced by AxisAlignedReflect
    # Plane and Center have been replaced by a vtkPlane 'ReflectionPlane'
    if proxy.SMProxy and proxy.SMProxy.GetXMLName() in ("ReflectionFilter", "AxisAlignedReflect"):
        if pname == "Plane":
            if compatibility_version < (6, 0):
                if type(value).__name__ == 'str':
                    if value == 'X Min':
                        value = 0
                    elif value == 'Y Min':
                        value = 1
                    elif value == 'Z Min':
                        value = 2
                    elif value == 'X Max':
                        value = 3
                    elif value == 'Y Max':
                        value = 4
                    elif value == 'Z Max':
                        value = 5
                    elif value == 'X':
                        value = 6
                    elif value == 'Y':
                        value = 7
                    elif value == 'Z':
                        value = 8
                if value < 6:
                    proxy.PlaneMode = value + 1
                else:
                    proxy.PlaneMode = 0
                    if value == 6:
                        proxy.GetProperty("ReflectionPlane").GetData().Normal = [1, 0, 0]
                    elif value == 7:
                        proxy.GetProperty("ReflectionPlane").GetData().Normal = [0, 1, 0]
                    elif value == 8:
                        proxy.GetProperty("ReflectionPlane").GetData().Normal = [0, 0, 1]
                raise Continue()
            else:
                raise NotSupportedException("'Plane' property has been removed in ParaView 6.0. Please use ReflectionPlane to define the plane instead.")
        if pname == "Center":
            if compatibility_version < (6, 0):
                proxy.GetProperty("ReflectionPlane").GetData().Origin = [value, value, value]
            else:
                raise NotSupportedException("'Center' property has been removed in ParaView 6.0. Please use ReflectionPlane to define the plane instead.")
        if pname == "FlipAllInputArrays":
            if compatibility_version < (6, 0):
                proxy.GetProperty("ReflectAllInputArrays").SetData(value)
            else:
                raise NotSupportedException("'FlipAllInputArrays' was renamed in 'ReflectAllInputArrays' since ParaView 6.0")

    # 6.0 -> 6.1 vtkParticleTracerBase have been reworked
    # Caching is now automated, DisableResetCache has been removed
    # TerminationTime is not exposed anymore
    if pname == "DisableResetCache" and proxy.SMProxy.GetXMLName() in ["LegacyStreakLine", "StreakLine"]:
        if compatibility_version <= (6, 0):
            return
        else:
            raise NotSupportedException(
                "%s now automates caching, simply remove DisableResetCache usages." % proxy.SMProxy.GetXMLName())
    if pname == "TerminationTime" and proxy.SMProxy.GetXMLName() == "ParticlePath":
        if compatibility_version <= (6, 0):
            return
        else:
            raise NotSupportedException(
                "ParticlePath does not support setting TerminationTime anymore, simply remove it.")

    chart_proxies = ["ImageChartRepresentation", "XYChartRepresentationBase", "XYChartRepresentation",
                     "XYPointChartRepresentation", "XYBarChartRepresentation", "QuartileChartRepresentation",
                     "ParallelCoordinatesRepresentation", "BoxChartRepresentation", "PlotMatrixRepresentation",
                     "BagPlotMatrixRepresentation", "XYBagChartRepresentation", "XYFunctionalBagChartRepresentation"]
    if pname == "CompositeDataSetIndex" and proxy.SMProxy.GetXMLName() in chart_proxies:
        if compatibility_version <= (6, 0):
            selectors = ["//*[@cid='%s']" % cid for cid in value]
            return proxy.GetProperty("BlockSelectors").SetData(selectors)
        else:
            raise NotSupportedException(
                "SpreadSheetRepresentation no longer uses 'CompositeDataSetIndex' but instead "
                "supports 'Selectors' to select blocks.")

    if not hasattr(proxy, pname):
        raise AttributeError()
    proxy.__dict__[pname] = value

    raise Continue()


def setattr_fix_value(proxy, pname, value, setter_func):
    compatibility_version = get_paraview_compatibility_version()

    if pname == "ShaderPreset" and proxy.SMProxy.GetXMLName().endswith("Representation"):
        if value == "Gaussian Blur (Default)":
            if compatibility_version <= (5, 5):
                paraview.print_warning(
                    "The 'Gaussian Blur (Default)' option has been renamed to 'Gaussian Blur'.  Please use that"
                    "instead.")
                setter_func(proxy, "Gaussian Blur")
                raise Continue()
            else:
                raise NotSupportedException("'Gaussian Blur (Default)' is an obsolete value for ShaderPreset. "
                                            " Use 'Gaussian Blur' instead.")

    if pname == "FieldAssociation" and proxy.SMProxy.GetXMLName() in ["DataSetCSVWriter", "CSVWriter"]:
        if compatibility_version < (5, 8):
            if value == "Points":
                value = "Point Data"
            elif value == "Cells":
                value = "Cell Data"
            setter_func(proxy, value)
            raise Continue()
        else:
            raise NotSupportedException("'FieldAssociation' is using an obsolete "
                                        "value '%s', use `Point Data` or `Cell Data` instead." % value)

    # In 5.9, we changed "High Resolution Line Source" to "Line" and "Point Source" to
    # "Point Cloud"
    seed_sources_from = ["High Resolution Line Source", "Point Source"]
    seed_sources_to = ["Line", "Point Cloud"]
    seed_sources_proxyname = ["HighResLineSource", "PointSource"]
    if value in seed_sources_from and proxy.GetProperty(pname).SMProperty.IsA("vtkSMInputProperty"):
        domain = proxy.GetProperty(pname).SMProperty.FindDomain("vtkSMProxyListDomain")
        for i in range(len(seed_sources_to)):
            if value == seed_sources_from[i]:
                domain_proxy = domain.FindProxy("extended_sources", seed_sources_proxyname[i])
                if domain_proxy:
                    if compatibility_version < (5, 9):
                        value = seed_sources_to[i]
                        setter_func(proxy, value)
                        raise Continue()
                    else:
                        raise NotSupportedException(
                            "%s is an obsolete value. Use %s instead." % (seed_sources_from[i], seed_sources_to[i]))

    if (pname == "WindowLocation" and proxy.SMProxy.GetXMLName() in ["TextSourceRepresentation",
                                                                     "ScalarBarWidgetRepresentation"]) or \
            (pname == "LabelLocation" and proxy.SMProxy.GetXMLName() == "ChartTextRepresentation"):
        # In ParaView 5.10, we changed "WindowsLocation/LabelLocation" values to have spaces in between as seen in
        # the following map
        window_location_map = {'AnyLocation': 'Any Location', 'LowerRightCorner': 'Lower Right Corner',
                               'LowerLeftCorner': 'Lower Left Corner', 'LowerCenter': 'Lower Center',
                               'UpperLeftCorner': 'Upper Left Corner', 'UpperRightCorner': 'Upper Right Corner',
                               'UpperCenter': 'Upper Center'}
        new_value = window_location_map[value]
        if compatibility_version <= (5, 9):
            setter_func(proxy, new_value)
            raise Continue()
        else:
            raise NotSupportedException("%s is an obsolete value. Use %s instead." % (value, new_value))

    if pname == "PointMergeMethod" and proxy.SMProxy.GetXMLName() in ["Contour", "Cut", "GenericContour"]:
        if compatibility_version < (5, 12):
            if value == "Don't Merge Points":
                new_value = "NotMergingPoints"
            setter_func(proxy, new_value)
            raise Continue()
        else:
            raise NotSupportedException("%s is an obsolete value. Use %s instead." % (value, new_value))

    # Always keep this line last
    raise ValueError("'%s' is not a valid value for %s!" % (value, pname))


def getattr(proxy, pname):
    """
    Attempts to emulate getattr() when called using a deprecated property name
    for a proxy.

    Will return a *reasonable* stand-in if the property was
    deprecated and the paraview compatibility version was set to a version older
    than when the property was deprecated.

    Will raise ``NotSupportedException`` if the property was deprecated and
    paraview compatibility version is newer than that deprecation version.

    Will raise ``Continue`` to indicate the property name is unaffected by
    any API deprecation and the caller should follow normal code execution
    paths.
    """
    compatibility_version = get_paraview_compatibility_version()

    # In 4.2, we removed ColorAttributeType property. One is expected to use
    # ColorArrayName to specify the attribute type as well.
    if pname == "ColorAttributeType" and proxy.SMProxy.GetProperty("ColorArrayName"):
        if compatibility_version <= (4, 1):
            if proxy.GetProperty("ColorArrayName")[0] == "CELLS":
                return "CELL_DATA"
            else:
                return "POINT_DATA"
        else:
            # if ColorAttributeType is being used, warn.
            raise NotSupportedException(
                "'ColorAttributeType' is obsolete. Simply use 'ColorArrayName' instead.  Refer to ParaView Python API "
                "changes documentation online.")

    # In 5.1, we removed CameraClippingRange property. It was not of any use
    # since we added support to render view to automatically reset clipping
    # range for each render.
    if pname == "CameraClippingRange" and not proxy.SMProxy.GetProperty("CameraClippingRange"):
        if compatibility_version <= (5, 0):
            return [0.0, 0.0, 0.0]
        else:
            raise NotSupportedException(
                'CameraClippingRange is obsolete. Please remove '
                'it from your script. You no longer need it.')

    # In 5.1, we remove support for Cube Axes and related properties.
    global _ACubeAxesHelper
    if proxy.SMProxy.IsA("vtkSMPVRepresentationProxy") and hasattr(_ACubeAxesHelper, pname):
        if compatibility_version <= (5, 0):
            return builtins.getattr(_ACubeAxesHelper, pname)
        else:
            raise NotSupportedException(
                'Cube Axes and related properties are now obsolete. Please '
                'remove them from your script.')

    # In 5.4, we removed the AspectRatio property and replaced it with the
    # ScalarBarThickness property.
    if pname == "AspectRatio" and proxy.SMProxy.GetProperty("ScalarBarThickness"):
        if compatibility_version <= (5, 3):
            return 20.0
        else:
            raise NotSupportedException(
                'The AspectRatio property has been removed in ParaView '
                '5.4. Please use the ScalarBarThickness property instead '
                'to set the thickness in terms of points.')

    # In 5.4, we removed the Position2 property and replaced it with the
    # ScalarBarLength property.
    if pname == "Position2" and proxy.SMProxy.GetProperty("ScalarBarLength"):
        if compatibility_version <= (5, 3):
            if proxy.GetProperty("Orientation").GetData() == "Horizontal":
                return [0.05, proxy.GetProperty("ScalarBarLength").GetData()]
            else:
                return [proxy.GetProperty("ScalarBarLength").GetData(), 0.05]
        else:
            raise NotSupportedException(
                'The Position2 property has been removed in ParaView '
                '5.4. Please set the ScalarBarLength property instead.')

    # In 5.5, we removed the PVLookupTable.LockScalarRange boolean property and
    # replaced it with the enumeration AutomaticRescaleRangeMode.
    if pname == "LockScalarRange" and proxy.SMProxy.GetProperty("AutomaticRescaleRangeMode"):
        if compatibility_version <= (5, 4):
            from paraview.modules.vtkRemotingViews import vtkSMTransferFunctionManager
            if proxy.GetProperty("AutomaticRescaleRangeMode").GetData() == "Never":
                return 1
            else:
                return 0
        else:
            raise NotSupportedException(
                'The PVLookupTable.LockScalarRange property has been removed '
                'in ParaView 5.5. Please set the AutomaticRescaleRangeMode property '
                'instead.')
    # In 5.5, we changed the vtkArrayCalculator to use a different set of constants to control which
    # data it operates on.  This change changed the method and property name from AttributeMode to
    # AttributeType
    if pname == "AttributeMode" and proxy.SMProxy.GetName() == "Calculator":
        if compatibility_version <= (5, 4):
            # The Attribute type uses enumeration values from vtkDataObject::AttributeTypes
            # rather than custom constants for the calculator.  For the values supported by
            # ParaView before this change, the conversion works out to adding 1 if it is an
            # integer. If the value is an enumerated string we use that as is since it matches
            # the previous enumerated string options.
            value = proxy.GetProperty("AttributeType").GetData()
            if isinstance(value, int):
                return value + 1
            return value
        else:
            raise NotSupportedException(
                'The Calculator.AttributeMode property has been removed in ParaView 5.5. '
                'Please set the AttributeType property instead. Note that different '
                'constants are needed for the two properties.')

    if pname == "UseOffscreenRenderingForScreenshots" and proxy.SMProxy.IsA("vtkSMViewProxy"):
        if compatibility_version <= (5, 4):
            return 0
        else:
            raise NotSupportedException('`UseOffscreenRenderingForScreenshots` '
                                        'is no longer supported. Please remove it.')

    if pname == "UseOffscreenRendering" and proxy.SMProxy.IsA("vtkSMViewProxy"):
        if compatibility_version <= (5, 4):
            return 0
        else:
            raise NotSupportedException(
                '`UseOffscreenRendering` is no longer supported. Please remove it.')

    if proxy.SMProxy.GetXMLName() == "CGNSSeriesReader" and \
            pname in ["LoadBndPatch", "LoadMesh", "BaseStatus", "FamilyStatus"]:
        if compatibility_version < (5, 5):
            if pname in ["LoadMesh", "LoadBndPatch"]:
                return 0
            else:
                paraview.print_warning(
                    "'%s' is no longer supported and will have no effect. "
                    "Please use `Blocks` property to specify blocks to load." % pname)
                return []
        else:
            raise NotSupportedException(
                "'%s' is obsolete. Use `Blocks` to make block based selection." % pname)

    # In 5.5, we removed the DataBoundsInflateFactor property and replaced it with the
    # DataBoundsScaleFactor property.
    if pname == "DataBoundsInflateFactor" and proxy.SMProxy.GetProperty("DataBoundsScaleFactor"):
        if compatibility_version <= (5, 4):
            inflateValue = proxy.GetProperty("DataBoundsScaleFactor").GetData() - 1
            if inflateValue >= 0:
                return inflateValue
            else:
                return 0
        else:
            raise NotSupportedException(
                'The  DataBoundsInflateFactorproperty has been removed in ParaView '
                '5.4. Please use the DataBoundsScaleFactor property instead.')

    if proxy.SMProxy and proxy.SMProxy.GetXMLName() == "AnnotateAttributeData":
        # in 5.5, Annotate Attribute Data changed how it sets the array to annotate
        if pname == "ArrayAssociation":
            if compatibility_version <= (5, 4):
                paraview.print_warning(
                    "'ArrayAssociation' is obsolete.  Use 'SelectInputArray' property of AnnotateAttributeData instead."
                )

                value = proxy.GetProperty("SelectInputArray")[0]
                if value == "CELLS":
                    return "Cell Data"
                elif value == "FIELD":
                    return "Field Data"
                elif value == "ROWS":
                    return "Row Data"
                else:
                    return "Point Data"
            else:
                raise NotSupportedException(
                    "'ArrayAssociation' is obsolete as of ParaView 5.5.  Use 'SelectInputArray' instead.")
        elif pname == "ArrayName":
            if compatibility_version <= (5, 4):
                paraview.print_warning(
                    "'ArrayName' is obsolete.  Use 'SelectInputArray' property of AnnotateAttributeData instead.")
                return proxy.GetProperty("SelectInputArray")[1]
            else:
                raise NotSupportedException(
                    "'ArrayName' is obsolete as of ParaView 5.5.  Use 'SelectInputArray' instead.")

    # In 5.5, we changed the Clip to be inverted from what it was before and changed the InsideOut
    # property to be called Invert to be clearer.
    if pname == "InsideOut" and proxy.SMProxy.GetXMLName() == "Clip":
        if compatibility_version <= (5, 4):
            return proxy.GetProperty("Invert").GetData()
        else:
            raise NotSupportedException(
                'The Clip.InsideOut property has been changed in ParaView 5.5. '
                'Please set the Invert property instead.')

    # In 5.6, we changed the "SpreadSheetRepresentation" proxy to no longer have
    # the "FieldAssociation" and "GenerateCellConnectivity" properties. They are
    # now moved to the view.
    if pname in ["FieldAssociation",
                 "GenerateCellConnectivity"] and proxy.SMProxy.GetXMLName() == "SpreadSheetRepresentation":
        if compatibility_version <= (5, 5):
            return 0
        else:
            raise NotSupportedException(
                "'%s' is obsolete on SpreadSheetRepresentation as of ParaView 5.6 and has been migrated to the view."
                % pname)

    # In 5.7, we changed to the names of the input proxies in ResampleWithDataset to clarify what
    # each source does.
    if pname == "Input" and proxy.SMProxy.GetXMLName() == "ResampleWithDataset":
        if compatibility_version < (5, 7):
            return proxy.GetProperty("SourceDataArrays")
        else:
            raise NotSupportedException(
                'The ResampleWithDataset.Input property has been changed in ParaView 5.7. '
                'Please access the SourceDataArrays property instead.')

    if pname == "Source" and proxy.SMProxy.GetXMLName() == "ResampleWithDataset":
        if compatibility_version < (5, 7):
            return proxy.GetProperty("DestinationMesh")
        else:
            raise NotSupportedException(
                'The ResampleWithDataset.Source property has been changed in ParaView 5.7. '
                'Please access the DestinationMesh property instead.')

    # In 5.7, we removed `ArrayName` property on the `GenerateIdScalars` filter
    # and replaced it with `CellIdsArrayName` and `PointIdsArrayName`.
    if pname == "ArrayName" and proxy.SMProxy.GetXMLName() in ("GenerateIdScalars", "PointAndCellIds"):
        if compatibility_version < (5, 7):
            return proxy.GetProperty("PointIdsArrayName")
        else:
            raise NotSupportedException(
                'The GenerateIdScalars.ArrayName property has been removed in ParaView 5.7. '
                'Please access `PointIdsArrayName` or `CellIdsArrayName` property instead.')

    # In 5.7, we renamed the 3D View's ray tracing interface from OSPRay to RayTracing
    if pname == "EnableOSPRay" and proxy.SMProxy.IsA("vtkSMRenderViewProxy"):
        if compatibility_version < (5, 7):
            return proxy.GetProperty("EnableRayTracing")
        else:
            raise NotSupportedException(
                'The `EnableOSPRay` control has been renamed in ParaView 5.7 to `EnableRayTracing`.')
    if pname == "OSPRayRenderer" and proxy.SMProxy.IsA("vtkSMRenderViewProxy"):
        if compatibility_version < (5, 7):
            return proxy.GetProperty("BackEnd")
        else:
            raise NotSupportedException(
                'The `OSPRayRenderer` control has been renamed in ParaView 5.7 to `BackEnd` and '
                'the settings `scivis` and `pathtracer` have been renamed to `OSPRay scivis` '
                'and `OSPRay pathtracer` respectively.')
    if pname == "OSPRayTemporalCacheSize" and proxy.SMProxy.IsA("vtkSMRenderViewProxy"):
        if compatibility_version < (5, 7):
            return proxy.GetProperty("TemporalCacheSize")
        else:
            raise NotSupportedException(
                'The `OSPRayTemporalCacheSize` control has been renamed in ParaView 5.7 to `TemporalCacheSize`.')
    if pname == "OSPRayUseScaleArray" and proxy.SMProxy.IsA("vtkSMRepresentationProxy"):
        if compatibility_version < (5, 7):
            return proxy.GetProperty("UseScaleArray")
        else:
            raise NotSupportedException(
                'The `OSPRayUseScaleArray` control has been renamed in ParaView 5.7 to `UseScaleArray`.')
    if pname == "OSPRayScaleFunction" and proxy.SMProxy.IsA("vtkSMRepresentationProxy"):
        if compatibility_version < (5, 7):
            return proxy.GetProperty("ScaleFunction")
        else:
            raise NotSupportedException(
                'The `OSPRayScaleFunction` control has been renamed in ParaView 5.7 to `ScaleFunction`.')
    if pname == "OSPRayMaterial" and proxy.SMProxy.IsA("vtkSMRepresentationProxy"):
        if compatibility_version < (5, 7):
            return proxy.GetProperty("Material")
        else:
            raise NotSupportedException(
                'The `OSPRayMaterial` control has been renamed in ParaView 5.7 to `Material`.')

    #  In 5.7, the `Box` implicit function's Scale property was renamed to
    #  Length.
    if pname == "Scale" and proxy.SMProxy.GetXMLName() == "Box":
        if compatibility_version < (5, 7):
            return proxy.GetProperty("Length")
        else:
            raise NotSupportedException(
                'The `Scale` property has been renamed in ParaView 5.7 to `Length`.')

    # In 5.9, CGNSSeriesReader no longer supports the "Blocks" property.
    if pname == "Blocks" and proxy.SMProxy.GetXMLName() == "CGNSSeriesReader":
        if compatibility_version < (5, 9):
            return []
        else:
            raise NotSupportedException(
                "The 'Blocks' property has been removed in ParaView 5.9. Use "
                "'Bases' to choose bases and 'Families' to choose families "
                "to load instead. 'LoadMesh' and 'LoadPatches' may also be "
                "used to enable loading of meshes and BC-patches.")

    # 5.10 onwards SpreadSheetRepresentation cannot provide a value for
    # CompositeDataSetIndex
    if pname == "CompositeDataSetIndex" and proxy.SMProxy.GetXMLName() == "SpreadSheetRepresentation":
        if compatibility_version <= (5, 9):
            return []
        else:
            raise NotSupportedException(
                "Since ParaView 5.10, SpreadSheetRepresentation no longer "
                "supports 'CompositeDataSetIndex' and it has been replaced by "
                "'BlockVisibilities'.")

    if proxy.SMProxy.GetXMLName() == "ExtractBlock":
        if pname == "BlockIndices":
            if compatibility_version <= (5, 9):
                return []
            else:
                raise NotSupportedException(
                    "Since ParaView 5.10, 'BlockIndices' on 'ExtractBlock' "
                    "has been replaced by 'Selectors'.")
        elif pname == "PruneOutput":
            if compatibility_version <= (5, 9):
                return 1
            else:
                raise NotSupportedException(
                    "Since ParaView 5.10, 'PruneOutput' on 'ExtractBlock' "
                    "is no longer supported. Simply remove it.")
        elif pname == "MaintainStructure":
            if compatibility_version <= (5, 9):
                return 1
            else:
                raise NotSupportedException(
                    "Since ParaView 5.10, 'MaintainStructure' on 'ExtractBlock' "
                    "is no longer supported. Simply remove it.")

    if pname in ["UseGradientBackground", "UseTexturedBackground",
                 "UseSkyboxBackground"] and proxy.SMProxy.GetXMLGroup() == "views" \
            and proxy.SMProxy.GetProperty("BackgroundColorMode"):
        if compatibility_version <= (5, 9):
            mode = proxy.GetProperty("BackgroundColorMode").GetData()
            if pname == "UseGradientBackground":
                return 1 if mode == "Gradient" else 0
            if pname == "UseTexturedBackground":
                return 1 if mode == "Texture" else 0
            if pname == "UseSkyboxBackground":
                return 1 if mode == "Skybox" else 0
        else:
            raise NotSupportedException(
                "Since ParaView 5.10, '%s' is no longer supported. Use "
                "'BackgroundColorMode' instead." % pname)

    if pname == "ThresholdRange" and proxy.SMProxy.GetXMLName() == "Threshold":
        # The Threshold filter now offers additional thresholding methods
        # besides ThresholdBetween. The lower and upper threshold values
        # are also set separately.
        if compatibility_version <= (5, 9):
            return [proxy.GetProperty("LowerThreshold").GetData(),
                    proxy.GetProperty("UpperThreshold").GetData()]
        else:
            raise NotSupportedException("The 'ThresholdRange' property has been "
                                        "removed in ParaView 5.10. Please set the lower and upper "
                                        "thresholds using the 'LowerThreshold' and 'UpperThreshold' "
                                        "properties, then set the 'ThresholdMethod' property to "
                                        "'Between' to threshold between the lower and upper thresholds.")

    if pname == "ThresholdBetween" and proxy.SMProxy.GetXMLName() == "VTKmThreshold":
        # The Threshold filter now offers additional thresholding methods
        # besides ThresholdBetween. The lower and upper threshold values
        # are also set separately.
        if compatibility_version <= (5, 10):
            return [proxy.GetProperty("LowerThreshold").GetData(),
                    proxy.GetProperty("UpperThreshold").GetData()]
        else:
            raise NotSupportedException("The 'ThresholdRange' property has been "
                                        "removed in ParaView 5.11. Please set the lower and upper "
                                        "thresholds using the 'LowerThreshold' and 'UpperThreshold' "
                                        "properties, then set the 'ThresholdMethod' property to "
                                        "'Between' to threshold between the lower and upper thresholds.")

    if pname == "CutoffArray" and proxy.SMProxy.GetXMLName() != "SPHDataSetInterpolator":
        raise NotSupportedException("The 'CutoffArray' property has been removed in ParaView 5.11."
                                    "If you are wishing to use this property, please use 'SPHDataSetInterpolator' "
                                    "instead. The 'CutoffArray' needs to be provided by the data set source.")

    if pname == "StaticMesh" and proxy.SMProxy.GetXMLName() in ["ParticleTracer, ParticlePath, StreakLine"]:
        if compatibility_version <= (5, 10):
            return proxy.GetProperty("MeshOverTime").GetData()
        else:
            raise NotSupportedException(
                "The " + proxy.SMProxy.GetXMLName() + ".StaticMesh property has been changed in ParaView 5.11. "
                                                      "Please set the MeshOverTime property instead.")

    if proxy.SMProxy.GetXMLName() == "DataSetSurfaceFilter":
        if pname == "UseGeometryFilter":
            if compatibility_version <= (5, 10):
                return 1
            else:
                raise NotSupportedException(
                    "Since ParaView 5.11, 'UseGeometryFilter' has been removed. ")

    # 5.11 -> 5.12 breaking changes on "TableFFT" properties
    # Renamed AverageFFTperblock into UseWelchMethod
    # Renamed OptimizeForRealInput into OneSidedSpectrum
    # Removed NumberOfBlock
    if proxy.SMProxy and proxy.SMProxy.GetXMLName() == "TableFFT":
        if pname == "AverageFFTperblock":
            if compatibility_version < (5, 12):
                return proxy.GetProperty("UseWelchMethod").GetData()
            else:
                raise NotSupportedException("'AverageFft' is obsolete.  Use 'UseWelchMethod' property instead.")
        elif pname == "OptimizeForRealInput":
            if compatibility_version < (5, 12):
                return proxy.GetProperty("OneSidedSpectrum").GetData()
            else:
                raise NotSupportedException(
                    "'OptimizeForRealInput' is obsolete.  Use 'OneSidedSpectrum' property instead.")
        elif pname == "NumberOfBlock" and not compatibility_version < (5, 12):
            raise NotSupportedException("'NumberOfBlock' is obsolete.  See 'BlockOverlap' property instead.")

    if proxy.SMProxy and proxy.SMProxy.GetXMLName() == "Cut":
        if pname == "MergePoints":
            if compatibility_version < (5, 12):
                return proxy.GetProperty("Locator").GetData().SMProxy.GetXMLLabel() == "Uniform Binning"
            else:
                raise NotSupportedException("'MergePoints' is obsolete.  Use 'Locator' property instead.")

    if proxy.SMProxy and proxy.SMProxy.GetXMLName() == "AnimationScene":
        if pname == "PlayMode" and proxy.GetProperty(pname).GetData() == "Real Time":
            raise NotSupportedException("'Real Time' is an obsolete value for 'PlayMode'. Use 'Sequence' instead.")

    # 5.12 -> 5.13 breaking change on vtkOpenFoamReader
    # DecomposePolyhedra property has been removed
    if proxy.SMProxy and proxy.SMProxy.GetXMLName() == "OpenFOAMReader":
        if pname == "DecomposePolyhedra":
            if compatibility_version < (5, 13):
                paraview.print_warning("'%s' is no longer supported and will have no effect. " % pname)
                raise Continue()
            else:
                raise NotSupportedException("'DecomposePolyhedra' property has been removed in ParaView 5.13")

    # 5.12 -> 5.13 breaking change on Representation properties
    # Renamed Position into Translation
    if proxy.SMProxy and proxy.SMProxy.GetXMLName().endswith("Representation"):
        if pname == "Position":
            if compatibility_version < (5, 13):
                return proxy.GetProperty("Translation").GetData()
            else:
                raise NotSupportedException("'Position' property has been removed in ParaView 5.13")


    # 5.13 -> 6.0 breaking change in PolarAxes representation
    # Properties Renaming
    if proxy.SMProxy and proxy.SMProxy.GetXMLName() == "PolarAxesRepresentation":
        # "NumberOfPolarAxes" -> "NumberOfArcs"
        if pname == "NumberOfPolarAxes":
            if compatibility_version < (6, 0):
                return proxy.GetProperty("NumberOfArcs").GetData()
            else:
                raise NotSupportedException("'NumberOfPolarAxes' was renamed in 'NumberOfArcs' since ParaView 6.0")

        # "DeltaRangePolarAxes" -> "DeltaRangeArcs"
        if pname == "DeltaRangePolarAxes":
            if compatibility_version < (6, 0):
                return proxy.GetProperty("DeltaRangeArcs").GetData()
            else:
                raise NotSupportedException("'DeltaRangePolarAxes' was renamed in 'DeltaRangeArcs' since ParaView 6.0")

        # "RadialTitleVisibility" -> "RadialLabelVisibility"
        if pname == "RadialTitleVisibility":
            if compatibility_version < (6, 0):
                return proxy.GetProperty("RadialLabelVisibility").GetData()
            else:
                raise NotSupportedException("'RadialTitleVisibility' was renamed in 'RadialLabelVisibility' since ParaView 6.0")

        # "RadialTitleFormat" -> "RadialLabelFormat"
        if pname == "RadialTitleFormat":
            if compatibility_version < (6, 0):
                return proxy.GetProperty("RadialLabelFormat").GetData()
            else:
                raise NotSupportedException("'RadialTitleFormat' was renamed in 'RadialLabelFormat' since ParaView 6.0")

        # "RadialTitleLocation" -> "RadialLabelLocation"
        if pname == "RadialTitleLocation":
            if compatibility_version < (6, 0):
                return proxy.GetProperty("RadialLabelLocation").GetData()
            else:
                raise NotSupportedException("'RadialTitleLocation' was renamed in 'RadialLabelLocation' since ParaView 6.0")

        # "RadialTitleOffset" -> "RadialLabelOffset"
        if pname == "RadialTitleOffset":
            if compatibility_version < (6, 0):
                return proxy.GetProperty("RadialLabelOffset").GetData()
            else:
                raise NotSupportedException("'RadialTitleOffset' was renamed in 'RadialLabelOffset' since ParaView 6.0")

        # "PolarTicksVisibility" -> "AllTicksVisibility"
        if pname == "PolarTicksVisibility":
            if compatibility_version < (6, 0):
                return proxy.GetProperty("AllTicksVisibility").GetData()
            else:
                raise NotSupportedException("'PolarTicksVisibility' was renamed in 'AllTicksVisibility' since ParaView 6.0")

    # 5.13 -> 6.0 HyperTreeGridAxisReflection replaced by AxisAlignedReflect
    # PlaneNormal and PlanePosition have been replaced by a vtkPlane 'ReflectionPlane'
    if proxy.SMProxy and proxy.SMProxy.GetXMLName() in ("HyperTreeGridAxisReflection", "AxisAlignedReflect"):
        if pname == "PlaneNormal":
            if compatibility_version < (6, 0):
                normal = proxy.GetProperty("ReflectionPlane").GetData().Normal
                if normal[0] == 1:
                    return 6
                if normal[1] == 1:
                    return 7
                if normal[2] == 1:
                    return 8
            else:
                raise NotSupportedException("'PlaneNormal' property has been removed in ParaView 6.0. Please use ReflectionPlane to define the plane instead.")
        if pname == "PlanePosition":
            if compatibility_version < (6, 0):
                normal = proxy.GetProperty("ReflectionPlane").GetData().Normal
                origin = proxy.GetProperty("ReflectionPlane").GetData().Origin
                if normal[0] == 1:
                    return origin[0]
                if normal[1] == 1:
                    return origin[1]
                if normal[2] == 1:
                    return origin[2]
            else:
                raise NotSupportedException("'PlanePosition' property has been removed in ParaView 6.0. Please use ReflectionPlane to define the plane instead.")

    # 5.13 -> 6.0 Reflect replaced by AxisAlignedReflect
    # Plane and Center have been replaced by a vtkPlane 'ReflectionPlane'
    if proxy.SMProxy and proxy.SMProxy.GetXMLName() in ("ReflectionFilter", "AxisAlignedReflect"):
        if pname == "Plane":
            if compatibility_version < (6, 0):
                planeMode = proxy.PlaneMode
                if planeMode == 'Interactive':
                    normal = proxy.GetProperty("ReflectionPlane").GetData().Normal
                    if normal[0] == 1:
                        return 6
                    if normal[1] == 1:
                        return 7
                    if normal[2] == 1:
                        return 8
                    return 1
                else:
                    return planeMode
            else:
                raise NotSupportedException("'Plane' property has been removed in ParaView 6.0. Please use ReflectionPlane to define the plane instead.")
        if pname == "Center":
            if compatibility_version < (6, 0):
                normal = proxy.GetProperty("ReflectionPlane").GetData().Normal
                origin = proxy.GetProperty("ReflectionPlane").GetData().Origin
                if normal[0] == 1:
                    return origin[0]
                if normal[1] == 1:
                    return origin[1]
                if normal[2] == 1:
                    return origin[2]
            else:
                raise NotSupportedException("'Center' property has been removed in ParaView 6.0. Please use ReflectionPlane to define the plane instead.")
        if pname == "FlipAllInputArrays":
            if compatibility_version < (6, 0):
                return proxy.GetProperty("ReflectAllInputArrays").GetData()
            else:
                raise NotSupportedException("'FlipAllInputArrays' was renamed in 'ReflectAllInputArrays' since ParaView 6.0")

    # 6.0 -> 6.1 onwards chart representations cannot provide a value for CompositeDataSetIndex
    chart_proxies = ["ImageChartRepresentation", "XYChartRepresentationBase", "XYChartRepresentation",
                     "XYPointChartRepresentation", "XYBarChartRepresentation", "QuartileChartRepresentation",
                     "ParallelCoordinatesRepresentation", "BoxChartRepresentation", "PlotMatrixRepresentation",
                     "BagPlotMatrixRepresentation", "XYBagChartRepresentation", "XYFunctionalBagChartRepresentation"]
    if pname == "CompositeDataSetIndex" and proxy.SMProxy.GetXMLName() in chart_proxies:
        if compatibility_version < (6, 1):
            return []
        else:
            raise NotSupportedException(
                "Since ParaView 6.1, chart representations no longer "
                "supports 'CompositeDataSetIndex' and it has been replaced by "
                "'BlockSelectors'.")

    raise Continue()


# Depending on the compatibility version that has been set, older functionalities
# are restored in this function. Note that the 'key' variable refers to a proxy
# label and not its name.
def GetProxy(module, key, **kwargs):
    compatibility_version = get_paraview_compatibility_version()

    if compatibility_version < (5, 5):
        if key == "Clip":
            # in PV 5.5 we changed the default for Clip's InsideOut property to 1 instead of 0
            # also InsideOut was changed to Invert in 5.5
            clip = builtins.getattr(module, key)(**kwargs)
            clip.Invert = 0
            return clip
    if compatibility_version < (5, 7):
        if key == "ExodusRestartReader" or key == "ExodusIIReader":
            # in 5.7, we changed the names for blocks, this preserves old
            # behavior
            reader = builtins.getattr(module, key)(**kwargs)
            reader.UseLegacyBlockNamesWithElementTypes = 1
            return reader
    if compatibility_version <= (5, 9):
        if key == "PlotOverLine":
            # in 5.10, we changed the backend of Plot Over Line
            # This restores the previous backend.
            probeLine = builtins.getattr(module, "PlotOverLineLegacy")(**kwargs)
            return probeLine
        if key in ["RenderView", "OrthographicSliceView", "ComparativeRenderView"]:
            view = builtins.getattr(module, key)(**kwargs)
            view.UseColorPaletteForBackground = 0
            return view
        if key == "Calculator":
            # In 5.10, we changed the array calculator's parser.
            # This restores the previous calculator expression parser
            # to handle syntax it supports.
            calculator = builtins.getattr(module, key)(**kwargs)
            calculator.FunctionParserType = 0
            return calculator
        if key == "Gradient":
            # In 5.10, 'Gradient' and 'GradientOfUnstructuredDataSet' were merged
            # into a unique 'Gradient" filter.
            gradient = builtins.getattr(module, "GradientLegacy")(**kwargs)
            return gradient
    if compatibility_version <= (5, 11):
        proxy = builtins.getattr(module, key)(**kwargs)
        if hasattr(proxy, "Assembly"):
            assemblyProperty = proxy.SMProxy.GetProperty("Assembly")
            if assemblyProperty:
                assemblyDomain = assemblyProperty.FindDomain("vtkSMDataAssemblyListDomain")
                if assemblyDomain:
                    assemblyDomain.SetBackwardCompatibilityMode(True)
                    return proxy
    if compatibility_version <= (5, 12):
        if key in ["ParticlePath", "StreakLine"]:
            return builtins.getattr(module, 'Legacy' + key)(**kwargs)
    if compatibility_version <= (5, 13):
        # In 5.13, in OpenFOAMReader we changed the default value of Createcelltopointfiltereddata to 0
        if key == "OpenFOAMReader":
            reader = builtins.getattr(module, key)(**kwargs)
            reader.Createcelltopointfiltereddata = 0
            return reader

    # deprecation case
    if type(key) == tuple and len(key) == 2:
        proxy = builtins.getattr(module, key[1])(**kwargs)

        return proxy

    return builtins.getattr(module, key)(**kwargs)


def get_deprecated_proxies(proxiesNS):
    """
    Provide a map between deprecated proxies and their fallback proxy
    The key is the previous name, value the new.
    By name we mean the actual python method name to construct the proxy,
    not the proxy name.
    Python method name is constructed from proxy label, sanitized to be
    a valid python method name.
    """
    compatibility_version = get_paraview_compatibility_version()
    proxies = {}
    proxies[proxiesNS.sources] = []
    proxies[proxiesNS.filters] = []

    if compatibility_version <= (5, 13):
        proxies[proxiesNS.filters] += [("GhostCellsGenerator", "GhostCells")]
        proxies[proxiesNS.filters] += [("AddFieldArrays", "FieldArraysFromFile")]
        proxies[proxiesNS.filters] += [("AppendArcLength", "PolylineLength")]
        proxies[proxiesNS.filters] += [("AppendLocationAttributes", "Coordinates")]
        proxies[proxiesNS.filters] += [("BlockScalars", "BlockIds")]
        proxies[proxiesNS.filters] += [("ComputeConnectedSurfaceProperties", "ConnectedSurfaceProperties")]
        proxies[proxiesNS.filters] += [("GenerateGlobalIds", "GlobalPointAndCellIds")]
        proxies[proxiesNS.filters] += [("GenerateIds", "PointAndCellIds")]
        proxies[proxiesNS.filters] += [("GenerateProcessIds", "ProcessIds")]
        proxies[proxiesNS.filters] += [("GenerateSpatioTemporalHarmonics", "SpatioTemporalHarmonics")]
        proxies[proxiesNS.filters] += [("GenerateSurfaceNormals", "SurfaceNormals")]
        proxies[proxiesNS.filters] += [("GenerateSurfaceTangents", "SurfaceTangents")]
        proxies[proxiesNS.filters] += [("LevelScalarsOverlappingAMR", "OverlappingAMRLevelIds")]

    if compatibility_version <= (6, 0):
        proxies[proxiesNS.filters] += [("HyperTreeGridCellCenters", "CellCenters")]
        proxies[proxiesNS.filters] += [("HyperTreeGridFeatureEdgesFilter", "FeatureEdges")]
        proxies[proxiesNS.filters] += [("HyperTreeGridGhostCellsGenerator", "GhostCells")]
        proxies[proxiesNS.filters] += [("HyperTreeGridAxisReflection", "AxisAlignedReflect")]
        proxies[proxiesNS.filters] += [("ProcessIdScalars", "ProcessIds")]
        proxies[proxiesNS.filters] += [("Reflect", "AxisAlignedReflect")]
        proxies[proxiesNS.filters] += [("HyperTreeGridVisibleLeavesSize", "HyperTreeGridGenerateFields")]

    return proxies


def lookupTableUpdate(lutName):
    """
    Provide backwards compatibility for color lookup table name changes.
    """
    # For backwards compatibility
    compatibility_version = get_paraview_compatibility_version()

    reverseLut = False
    if compatibility_version <= (5, 8):
        # In 5.9, some redundant color maps were removed and some had name changes.
        # Replace these with a color map that remains. Also handle some color map
        # name changes.
        reverse = ["Red to Blue Rainbow"]
        if lutName in reverse:
            reverseLut = True
        nameChanges = {
            "jet": "Jet",
            "coolwarm": "Cool to Warm",
            "Asymmtrical Earth Tones (6_21b)": "Asymmetrical Earth Tones (6_21b)",
            "CIELab_blue2red": "CIELab Blue to Red",
            "gray_Matlab": "Grayscale",
            "rainbow": "Blue to Red Rainbow",
            "Red to Blue Rainbow": "Blue to Red Rainbow"
        }
        try:
            lutName = nameChanges[lutName]
        except:
            pass
    if compatibility_version <= (6, 0):
        # In 6.1, the string "(matplotlib)" was removed from some color preset names
        nameChanges = {
            "Inferno (matplotlib)": "Inferno",
            "Viridis (matplotlib)": "Viridis",
            "Magma (matplotlib)": "Magma",
            "Plasma (matplotlib)": "Plasma"
        }
        try:
            lutName = nameChanges[lutName]
        except:
            pass

    return lutName, reverseLut

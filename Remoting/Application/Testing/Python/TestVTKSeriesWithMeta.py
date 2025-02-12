from paraview.simple import *
from paraview import smtesting

# import paraview
# paraview.compatibility.major = 5
# paraview.compatibility.minor = 13

import os.path

smtesting.ProcessCommandLineArguments()

meta_file = os.path.join(smtesting.DataDir, "Testing/Data/FileSeries/blow.vtk.series")
LegacyVTKReader(FileNames=[meta_file])

warpByVector1 = WarpByVector()
warpByVector1.Vectors = ['POINTS', 'displacement']

Show()

animationScene1 = GetAnimationScene()
animationScene1.NumberOfFrames = 10
animationScene1.GoToNext()

tk=GetTimeKeeper()
assert(tk.Time == 2)

view = Render()

view.CameraPosition = [88.55391631920813, 2.2507631219000945, 12.701476271463322]
view.CameraFocalPoint = [3.499999999999998, 11.999999999999995, 0.9999999999999994]
view.CameraViewUp = [-0.13878261164731293, -0.022436576321793526, 0.9900686777932668]
view.CameraParallelScale = 27.060118255469618

if not smtesting.DoRegressionTesting(view.SMProxy):
    raise smtesting.TestError('Test failed.')

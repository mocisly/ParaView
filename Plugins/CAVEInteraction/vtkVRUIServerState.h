// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#ifndef vtkVRUIServerState_h
#define vtkVRUIServerState_h

#include "vtkSmartPointer.h"
#include "vtkVRUITrackerState.h"
#include <vector>

class vtkVRUIServerState
{
public:
  // Description:
  // Default constructor. All arrays are of size 0.
  vtkVRUIServerState();

  // Description:
  // Return the state of all the trackers.
  std::vector<vtkSmartPointer<vtkVRUITrackerState>>* GetTrackerStates();

  // Description:
  // Return the state of all the buttons.
  std::vector<bool>* GetButtonStates();

  // Description:
  // Return the state of all the valuators (whatever it is).
  std::vector<float>* GetValuatorStates();

protected:
  std::vector<vtkSmartPointer<vtkVRUITrackerState>> TrackerStates;
  std::vector<bool> ButtonStates;
  std::vector<float> ValuatorStates;

private:
  vtkVRUIServerState(const vtkVRUIServerState&) = delete;
  void operator=(const vtkVRUIServerState&) = delete;
};

#endif // #ifndef vtkVRUIServerState_h

// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSequenceAnimationPlayer.h"

#include "vtkObjectFactory.h"

#include <algorithm>

vtkStandardNewMacro(vtkSequenceAnimationPlayer);
//----------------------------------------------------------------------------
vtkSequenceAnimationPlayer::vtkSequenceAnimationPlayer()
{
  this->NumberOfFrames = 1;
  this->FrameNo = 0;
}

//----------------------------------------------------------------------------
vtkSequenceAnimationPlayer::~vtkSequenceAnimationPlayer() = default;

//----------------------------------------------------------------------------
void vtkSequenceAnimationPlayer::StartLoop(
  double starttime, double endtime, double vtkNotUsed(curtime), double* playbackWindow)
{
  // the frame index is inited to 0 ONLY when an animation is not resumed from
  // an intermediate frame
  this->FrameNo = 0;

  this->StartTime = starttime;
  this->EndTime = endtime;

  // currenttime, which might be the 'scene time' (usually unequal to
  // starttime) upon the previous pause / stop operation (if any), is used to
  // determine the actual frame index from which to resume the animation
  if (playbackWindow[0] > starttime)
  {
    // let's resume from the frame NEXT to the one on which the animation WAS
    // paused / stopped
    this->FrameNo = static_cast<int>((playbackWindow[0] - this->StartTime) *
                        (this->NumberOfFrames - 1) / (this->EndTime - this->StartTime) +
                      0.5) +
      1;

    // Let's compute the upper bounds in Frame unit
    this->MaxFrameWindow = static_cast<int>((playbackWindow[1] - this->StartTime) *
                               (this->NumberOfFrames - 1) / (this->EndTime - this->StartTime) +
                             0.5) +
      1;
  }
}

//----------------------------------------------------------------------------
double vtkSequenceAnimationPlayer::GetNextTime(double curtime)
{
  if (this->FrameNo == 0 && curtime > this->StartTime)
  {
    // Invalid Frame No, compute it correctly
    this->FrameNo = static_cast<int>((curtime - this->StartTime) * (this->NumberOfFrames - 1) /
                        (this->EndTime - this->StartTime) +
                      0.5) +
      1;
  }
  this->FrameNo += this->GetStride();
  if (this->StartTime >= this->EndTime && this->FrameNo >= this->MaxFrameWindow)
  {
    return VTK_DOUBLE_MAX;
  }

  return this->GetTimeFromTimestep(this->StartTime, this->EndTime, this->FrameNo);
}

//----------------------------------------------------------------------------
double vtkSequenceAnimationPlayer::GetPreviousTime(double curtime)
{
  if (this->FrameNo == 0 && curtime > this->StartTime)
  {
    // Invalid Frame No, compute it correctly
    this->FrameNo = static_cast<int>(
      (curtime - this->StartTime) * (this->NumberOfFrames - 1) / (this->EndTime - this->StartTime) +
      0.5);
  }
  this->FrameNo -= this->GetStride();
  if (this->StartTime >= this->EndTime && this->FrameNo <= 0)
  {
    return VTK_DOUBLE_MIN;
  }

  return this->GetTimeFromTimestep(this->StartTime, this->EndTime, this->FrameNo);
}

//----------------------------------------------------------------------------
int vtkSequenceAnimationPlayer::GetTimestep(double start, double end, double current)
{
  if (start == end)
  {
    return start;
  }

  return static_cast<int>((current - start) * (this->NumberOfFrames - 1) / (end - start) + 0.5);
}

//----------------------------------------------------------------------------
double vtkSequenceAnimationPlayer::GetTimeFromTimestep(double start, double end, int timestep)
{
  double delta = (end - start) / std::max(this->NumberOfFrames - 1, 1);
  return start + timestep * delta;
}

//----------------------------------------------------------------------------
double vtkSequenceAnimationPlayer::GoToNext(double start, double end, double curtime)
{
  int curTimestep = this->GetTimestep(start, end, curtime);
  double res = this->GetTimeFromTimestep(start, end, curTimestep + this->GetStride());
  return (res > end) ? curtime : res;
}

//----------------------------------------------------------------------------
double vtkSequenceAnimationPlayer::GoToPrevious(double start, double end, double curtime)
{
  int curTimestep = this->GetTimestep(start, end, curtime);
  double res = this->GetTimeFromTimestep(start, end, curTimestep - this->GetStride());
  return (res < start) ? curtime : res;
}

//----------------------------------------------------------------------------
void vtkSequenceAnimationPlayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

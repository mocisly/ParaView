// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAMRStreamingPriorityQueue.h"

#include "vtkBoundingBox.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMRMetaData.h"
#include "vtkStreamingPriorityQueue.h"

#include <cassert>
#include <queue>
#include <vector>

class vtkAMRStreamingPriorityQueue::vtkInternals
{
public:
  vtkStreamingPriorityQueue<> PriorityQueue;
  vtkSmartPointer<vtkOverlappingAMRMetaData> AMRMetadata;
};

vtkStandardNewMacro(vtkAMRStreamingPriorityQueue);
vtkCxxSetObjectMacro(vtkAMRStreamingPriorityQueue, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkAMRStreamingPriorityQueue::vtkAMRStreamingPriorityQueue()
{
  this->Internals = new vtkInternals();
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkAMRStreamingPriorityQueue::~vtkAMRStreamingPriorityQueue()
{
  delete this->Internals;
  this->Internals = nullptr;
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingPriorityQueue::Initialize(vtkOverlappingAMRMetaData* amr)
{
  delete this->Internals;
  this->Internals = new vtkInternals();
  this->Internals->AMRMetadata = amr;

  for (unsigned int cc = 0; cc < amr->GetNumberOfBlocks(); cc++)
  {
    vtkStreamingPriorityQueueItem item;
    item.Identifier = cc;
    item.Priority = (amr->GetNumberOfBlocks() - cc);

    unsigned int level = 0, index = 0;
    this->Internals->AMRMetadata->ComputeIndexPair(item.Identifier, level, index);
    item.Refinement = static_cast<double>(level);

    double block_bounds[6];
    this->Internals->AMRMetadata->GetBounds(level, index, block_bounds);
    item.Bounds.SetBounds(block_bounds);

    // default priority is to prefer lower levels. Thus even without
    // view-planes we have reasonable priority.
    this->Internals->PriorityQueue.push(item);
  }
}

//----------------------------------------------------------------------------
void vtkAMRStreamingPriorityQueue::Reinitialize()
{
  if (this->Internals->AMRMetadata)
  {
    vtkSmartPointer<vtkOverlappingAMRMetaData> info = this->Internals->AMRMetadata;
    this->Initialize(info);
  }
}

//----------------------------------------------------------------------------
bool vtkAMRStreamingPriorityQueue::IsEmpty()
{
  return this->Internals->PriorityQueue.empty();
}

//----------------------------------------------------------------------------
unsigned int vtkAMRStreamingPriorityQueue::Pop()
{
  if (this->IsEmpty())
  {
    vtkErrorMacro("Queue is empty!");
    return 0;
  }

  int num_procs = this->Controller ? this->Controller->GetNumberOfProcesses() : 1;
  int myid = this->Controller ? this->Controller->GetLocalProcessId() : 0;
  assert(myid < num_procs);

  std::vector<vtkStreamingPriorityQueueItem> items;
  items.resize(num_procs);
  for (int cc = 0; cc < num_procs && !this->Internals->PriorityQueue.empty(); cc++)
  {
    items[cc] = this->Internals->PriorityQueue.top();
    this->Internals->PriorityQueue.pop();
  }

  // at the end, when the queue empties out in the middle of a pop, right now,
  // all other processes are simply going to ask for block 0 (set in
  // initialization of vtkStreamingPriorityQueueItem). We can change that, if needed.
  return items[myid].Identifier;
}

//----------------------------------------------------------------------------
void vtkAMRStreamingPriorityQueue::Update(const double view_planes[24])
{
  double clamp_bounds[6];
  vtkMath::UninitializeBounds(clamp_bounds);
  this->Update(view_planes, clamp_bounds);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingPriorityQueue::Update(
  const double view_planes[24], const double clamp_bounds[6])
{
  if (!this->Internals->AMRMetadata)
  {
    return;
  }
  this->Internals->PriorityQueue.UpdatePriorities(view_planes, clamp_bounds);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingPriorityQueue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

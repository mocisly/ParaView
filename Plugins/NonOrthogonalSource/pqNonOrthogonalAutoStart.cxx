// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqNonOrthogonalAutoStart.h"
#include "pqModelTransformSupportBehavior.h"

//-----------------------------------------------------------------------------
pqNonOrthogonalAutoStart::pqNonOrthogonalAutoStart(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqNonOrthogonalAutoStart::~pqNonOrthogonalAutoStart() = default;

//-----------------------------------------------------------------------------
void pqNonOrthogonalAutoStart::startup()
{
  new pqModelTransformSupportBehavior(this);
}

//-----------------------------------------------------------------------------
void pqNonOrthogonalAutoStart::shutdown() {}

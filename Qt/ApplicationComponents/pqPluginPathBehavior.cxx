// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqPluginPathBehavior.h"

//-----------------------------------------------------------------------------
pqPluginPathBehavior::pqPluginPathBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
void pqPluginPathBehavior::loadDefaultPlugins(pqServer*) {}

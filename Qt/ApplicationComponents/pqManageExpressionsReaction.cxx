// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE

#include "pqManageExpressionsReaction.h"

#include "pqCoreUtilities.h"
#include "pqExpressionsDialog.h"

#include <QAction>
#include <QVariant>

pqManageExpressionsReaction::pqManageExpressionsReaction(QAction* action)
  : Superclass(action)
{
}

//-----------------------------------------------------------------------------
void pqManageExpressionsReaction::manageExpressions()
{
  pqExpressionsManagerDialog dialog(pqCoreUtilities::mainWidget());
  dialog.setObjectName("ExpressionsManagerDialog");
  dialog.exec();
}

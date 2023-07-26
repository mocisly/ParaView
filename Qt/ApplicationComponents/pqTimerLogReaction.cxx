// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqTimerLogReaction.h"

#include "pqTimerLogDisplay.h"
#include <QPointer>

//-----------------------------------------------------------------------------
void pqTimerLogReaction::showTimerLog()
{
  static QPointer<pqTimerLogDisplay> dialog;
  if (!dialog)
  {
    dialog = new pqTimerLogDisplay();
  }
  dialog->setAttribute(Qt::WA_QuitOnClose, false);
  dialog->show();
  dialog->raise();
  dialog->activateWindow();
  dialog->refresh();
}

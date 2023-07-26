// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#ifndef pqSaveStateAndScreenshotActions_h
#define pqSaveStateAndScreenshotActions_h

#include <QToolBar>
class pqSaveStateAndScreenshotActions : public QToolBar
{
  Q_OBJECT
public:
  pqSaveStateAndScreenshotActions(QWidget* p);

private:
  Q_DISABLE_COPY(pqSaveStateAndScreenshotActions)
};

#endif

// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE

/**
 * A main window class copied from SimpleParaView example
 * Make sure to propagate any changes mades
 */

#ifndef myMainWindow_h
#define myMainWindow_h

#include <QMainWindow>
#include <QScopedPointer>

class myMainWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;

public:
  myMainWindow();
  ~myMainWindow() override;

private:
  Q_DISABLE_COPY(myMainWindow)

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif

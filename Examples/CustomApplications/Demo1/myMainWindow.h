// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#ifndef myMainWindow_h
#define myMainWindow_h

#include <QMainWindow>

class myMainWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;

public:
  myMainWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
  ~myMainWindow();

protected Q_SLOTS:

private:
  Q_DISABLE_COPY(myMainWindow)
};

#endif

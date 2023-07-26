// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE

#ifndef LogViewerWidget_h
#define LogViewerWidget_h

#include <QObject>

class LogViewerWidgetTester : public QObject
{
  Q_OBJECT;
private Q_SLOTS:
  void basic();
};
#endif

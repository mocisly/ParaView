// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#ifndef pqPythonEventFilter_h
#define pqPythonEventFilter_h

#include <QEvent>
#include <QObject>

class pqPythonEventFilter : public QObject
{
  Q_OBJECT

public Q_SLOTS:

  void setEventHandlerResult(bool result) { this->EventHandlerResult = result; }

Q_SIGNALS:

  void handleEvent(QObject* obj, QEvent* event);

protected:
  bool eventFilter(QObject* obj, QEvent* event)
  {
    Q_EMIT this->handleEvent(obj, event);
    return this->EventHandlerResult;
  }

  bool EventHandlerResult;
};

#endif

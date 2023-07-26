// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE

#ifndef pqFlipBookToolbarActions_h
#define pqFlipBookToolbarActions_h

#include <QToolBar>

class pqFlipBookToolbarActions : public QToolBar
{
  Q_OBJECT

public:
  pqFlipBookToolbarActions(QWidget* p);

private:
  Q_DISABLE_COPY(pqFlipBookToolbarActions)
};

#endif

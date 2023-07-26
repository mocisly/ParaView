// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE

#include <QToolBar>

class pqMyToolBar : public QToolBar
{
  Q_OBJECT;
  using Superclass = QToolBar;

public:
  pqMyToolBar(const QString& title, QWidget* parent = nullptr);
  pqMyToolBar(QWidget* parent = nullptr);
  ~pqMyToolBar() override = default;

private:
  Q_DISABLE_COPY(pqMyToolBar);
  void constructor();
};

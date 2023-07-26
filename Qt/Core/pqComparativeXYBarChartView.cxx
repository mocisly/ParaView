// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqComparativeXYBarChartView.h"

//-----------------------------------------------------------------------------
pqComparativeXYBarChartView::pqComparativeXYBarChartView(const QString& group, const QString& name,
  vtkSMComparativeViewProxy* view, pqServer* server, QObject* parentObject)
  : Superclass(chartViewType(), group, name, view, server, parentObject)
{
}

//-----------------------------------------------------------------------------
pqComparativeXYBarChartView::~pqComparativeXYBarChartView() = default;

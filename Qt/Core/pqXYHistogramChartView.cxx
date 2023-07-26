// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqXYHistogramChartView.h"

#include "vtkSMContextViewProxy.h"

//-----------------------------------------------------------------------------
pqXYHistogramChartView::pqXYHistogramChartView(const QString& group, const QString& name,
  vtkSMContextViewProxy* viewModule, pqServer* server, QObject* p /*=nullptr*/)
  : Superclass(XYHistogramChartViewType(), group, name, viewModule, server, p)
{
}

//-----------------------------------------------------------------------------
pqXYHistogramChartView::~pqXYHistogramChartView() = default;

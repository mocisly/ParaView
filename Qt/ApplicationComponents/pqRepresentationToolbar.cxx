// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqRepresentationToolbar.h"

#include "pqActiveObjects.h"
#include "pqDisplayRepresentationWidget.h"
#include "pqSetName.h"

//-----------------------------------------------------------------------------
void pqRepresentationToolbar::constructor()
{
  this->setWindowTitle(tr("Representation Toolbar"));
  pqDisplayRepresentationWidget* widget = new pqDisplayRepresentationWidget(this)
    << pqSetName("displayRepresentation");
  this->addWidget(widget);
  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(representationChanged(pqDataRepresentation*)), widget,
    SLOT(setRepresentation(pqDataRepresentation*)));
}

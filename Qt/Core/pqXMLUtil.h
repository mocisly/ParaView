// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE

/**
 * \file pqXMLUtil.h
 *
 * \date 1/19/2006
 */

#ifndef pqXMLUtil_h
#define pqXMLUtil_h

#include "pqCoreModule.h"
#include <QList>
#include <QString>

class vtkPVXMLElement;

class PQCORE_EXPORT pqXMLUtil
{
public:
  static vtkPVXMLElement* FindNestedElementByName(vtkPVXMLElement* element, const char* name);

  static QString GetStringFromIntList(const QList<int>& list);
  static QList<int> GetIntListFromString(const char* value);
};

#endif

// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMArraySelectionDomain
 * @brief   used on properties that allow users to
 * select arrays.
 *
 * vtkSMArraySelectionDomain is a domain that can be for used for properties
 * that allow users to set selection-statuses for multiple arrays (or similar
 * items). This is similar to vtkSMArrayListDomain, the only different is that
 * vtkSMArrayListDomain is designed to work with data-information obtained
 * from the required Input property, while vtkSMArraySelectionDomain depends on
 * a required information-only property ("ArrayList") that provides the
 * arrays available.
 *
 * Supported Required-Property functions:
 * \li \c ArrayList : points a string-vector property that produces the
 * (array_name, status) tuples. This is typically an information-only property.
 */

#ifndef vtkSMArraySelectionDomain_h
#define vtkSMArraySelectionDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMStringListDomain.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMArraySelectionDomain : public vtkSMStringListDomain
{
public:
  static vtkSMArraySelectionDomain* New();
  vtkTypeMacro(vtkSMArraySelectionDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Since this domain relies on an information only property to get the default
   * status, we override this method to copy the values the info property as the
   * default array selection.
   */
  int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) override;

  /**
   * Global flag to toggle between (a) the default behavior of setting default
   * values according to infoProperty and (b) setting all default values to on.
   */
  static void SetLoadAllVariables(bool choice);
  static bool GetLoadAllVariables();

protected:
  vtkSMArraySelectionDomain();
  ~vtkSMArraySelectionDomain() override;

private:
  vtkSMArraySelectionDomain(const vtkSMArraySelectionDomain&) = delete;
  void operator=(const vtkSMArraySelectionDomain&) = delete;
};

#endif

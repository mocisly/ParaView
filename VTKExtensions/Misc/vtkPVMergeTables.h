// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVMergeTables
 * @brief   used to merge rows in tables.
 *
 * Simplified version of vtkMergeTables which simply combines tables merging
 * columns. This assumes that each of the inputs either has exactly identical
 * columns or no columns at all.
 * This filter can handle composite datasets as well. The output is produced by
 * merging corresponding leaf nodes. This assumes that all inputs have the same
 * composite structure.
 * All inputs must either be vtkTable or vtkCompositeDataSet mixing is not
 * allowed.
 * The output is a flattened vtkTable.
 * @todo
 * We may want to merge this functionality into vtkMergeTables filter itself.
 */

#ifndef vtkPVMergeTables_h
#define vtkPVMergeTables_h

#include "vtkPVVTKExtensionsMiscModule.h" // needed for export macro
#include "vtkSmartPointer.h"              // needed for vtkSmartPointer.
#include "vtkTableAlgorithm.h"

#include <vector> // needed for std::vector.

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkPVMergeTables : public vtkTableAlgorithm
{
public:
  static vtkPVMergeTables* New();
  vtkTypeMacro(vtkPVMergeTables, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkSmartPointer<vtkTable> MergeTables(const std::vector<vtkTable*>& tables);

protected:
  vtkPVMergeTables();
  ~vtkPVMergeTables() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  vtkExecutive* CreateDefaultExecutive() override;

  static std::vector<vtkTable*> GetTables(vtkInformationVector* inputVector);
  static void MergeTables(vtkTable* output, const std::vector<vtkTable*>& tables);

private:
  vtkPVMergeTables(const vtkPVMergeTables&) = delete;
  void operator=(const vtkPVMergeTables&) = delete;
};

#endif

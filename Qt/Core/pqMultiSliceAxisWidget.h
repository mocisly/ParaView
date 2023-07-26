// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#ifndef pqMultiSliceAxisWidget_h
#define pqMultiSliceAxisWidget_h

#include "pqCoreModule.h"

#include <QPointer>
#include <QWidget>

class vtkContextScene;
class vtkObject;

class PQCORE_EXPORT pqMultiSliceAxisWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;
  Q_PROPERTY(QString title READ title WRITE setTitle);

public:
  pqMultiSliceAxisWidget(QWidget* parent = nullptr);
  ~pqMultiSliceAxisWidget() override;

  /**
   * Set the range of the Axis (Bound)
   */
  void setRange(double min, double max);

  /**
   * Axis::LEFT=0, Axis::BOTTOM, Axis::RIGHT, Axis::TOP
   */
  void setAxisType(int type);

  /**
   * Title that appears inside the view
   */
  QString title() const;
  void setTitle(const QString& title);

  /**
   * Return the Widget that contain the ContextView
   */
  QWidget* getVTKWidget();

  /**
   * Return the locations of the visible slices within the range as well as
   * the number of values that can be read from the pointer
   */
  const double* getVisibleSlices(int& nbSlices) const;

  /**
   * Returns the locations for all slices (visible or otherwise).
   */
  const double* getSlices(int& nbSlices) const;

  /**
   * Update our internal model to reflect the proxy state
   */
  void updateSlices(double* values, bool* visibility, int numberOfValues);

  /**
   * The active size define the number of pixel that are going to be used for
   * the slider handle.
   */
  void SetActiveSize(int size);

  /**
   * The margin used on the side of the Axis.
   */
  void SetEdgeMargin(int margin);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void renderView();

Q_SIGNALS:
  /**
   * Signal emitted when the model has changed internally
   */
  void sliceAdded(int index);
  void sliceRemoved(int index);
  void sliceModified(int index);

  /**
   * Signal emitted when a mark is clicked. The value is inside the provided range
   */
  void markClicked(int button, int modifier, double value);

  void titleChanged(const QString&);

protected:
  vtkContextScene* scene() const;

  /**
   * Internal VTK callback used to emit the modelUpdated() signal
   */
  void invalidateCallback(vtkObject*, unsigned long, void*);

  /**
   * Internal VTK callback used to emit markClicked(...) signale
   */
  void onMarkClicked(vtkObject*, unsigned long, void*);

private:
  Q_DISABLE_COPY(pqMultiSliceAxisWidget)

  class pqInternal;
  pqInternal* Internal;
};

#endif

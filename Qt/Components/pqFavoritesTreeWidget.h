// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE

#ifndef pqFavoritesTreeWidget_h
#define pqFavoritesTreeWidget_h

#include <QTreeWidget>

#include <QSet>

#include "pqComponentsModule.h"

class QTreeWidgetItem;

/**
 * pqFavoritesTreeWidget is a custom widget used to display Favorites.
 * It extands a QTreeWidget.
 */
class PQCOMPONENTS_EXPORT pqFavoritesTreeWidget : public QTreeWidget
{
  typedef QTreeWidget Superclass;
  Q_OBJECT

public:
  pqFavoritesTreeWidget(QWidget* p = nullptr);

  bool isDropOnItem() { return this->dropIndicatorPosition() == QAbstractItemView::OnItem; }

protected:
  /**
   * Reimplemented to store unfolded dragged items.
   */
  void dragEnterEvent(QDragEnterEvent* event) override;

  /**
   * Reimplemented to keep unfolded items that were unfolded.
   */
  void dropEvent(QDropEvent* event) override;

  QSet<QTreeWidgetItem*> UnfoldedDraggedCategories;
};

#endif // !pqFavoritesTreeWidget_h

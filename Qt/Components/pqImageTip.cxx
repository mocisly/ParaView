// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqImageTip.h"

#include "vtkSetGet.h"

#include <QBasicTimer>
#include <QToolTip>
#include <qapplication.h>
#include <qdebug.h>
#include <qevent.h>
#include <qhash.h>
#include <qlabel.h>
#include <qpointer.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qstylepainter.h>
#include <qtimer.h>

void pqImageTip::showTip(const QPixmap& image, const QPoint& pos)
{
  static pqImageTip* instance = nullptr;

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
  const QPixmap* pixmap = instance ? instance->pixmap() : nullptr;
  bool pixmapIsNull = pixmap == nullptr;
  qint64 pixmapCacheKey = pixmap ? pixmap->cacheKey() : 0;
#else
  QPixmap pixmap = instance ? instance->pixmap(Qt::ReturnByValue) : QPixmap();
  bool pixmapIsNull = pixmap.isNull();
  qint64 pixmapCacheKey = pixmap.cacheKey();
#endif

  if (instance && instance->isVisible() && !pixmapIsNull && pixmapCacheKey == image.cacheKey())
    return;

  QToolTip::showText(QPoint(), "");

  delete instance;
  instance = new pqImageTip(image, nullptr);
  instance->move(pos + QPoint(2, 24));
  instance->show();
}

pqImageTip::pqImageTip(const QPixmap& image, QWidget* p)
  : QLabel(p, Qt::ToolTip)
  , hideTimer(new QBasicTimer())
{
  this->setPixmap(image);

  setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, nullptr, this));
  setFrameStyle(QFrame::NoFrame);
  setAlignment(Qt::AlignLeft);
  setIndent(1);
  ensurePolished();

  QFontMetrics fm(font());
  QSize extra(1, 0);
  // Make it look good with the default ToolTip font on Mac, which has a small descent.
  if (fm.descent() == 2 && fm.ascent() >= 11)
    ++extra.rheight();

  resize(sizeHint() + extra);
  qApp->installEventFilter(this);
  hideTimer->start(10000, this);
  setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, nullptr, this) / 255.0);
  // No resources for this yet (unlike on Windows).
  QPalette pal(Qt::black, QColor(255, 255, 220), QColor(96, 96, 96), Qt::black, Qt::black,
    Qt::black, QColor(255, 255, 220));
  setPalette(pal);
}

pqImageTip::~pqImageTip()
{
  delete this->hideTimer;
}

bool pqImageTip::eventFilter(QObject*, QEvent* e)
{
  switch (e->type())
  {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    {
      int key = static_cast<QKeyEvent*>(e)->key();
      Qt::KeyboardModifiers mody = static_cast<QKeyEvent*>(e)->modifiers();

      if ((mody & Qt::KeyboardModifierMask) ||
        (key == Qt::Key_Shift || key == Qt::Key_Control || key == Qt::Key_Alt ||
          key == Qt::Key_Meta))
      {
        break;
      }
      [[fallthrough]];
    }
    case QEvent::Leave:
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
      hide();
    default:;
  }
  return false;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void pqImageTip::enterEvent(QEvent*)
#else
void pqImageTip::enterEvent(QEnterEvent*)
#endif
{
  hide();
}

void pqImageTip::timerEvent(QTimerEvent* e)
{
  if (e->timerId() == hideTimer->timerId())
    hide();
}

void pqImageTip::paintEvent(QPaintEvent* ev)
{
  QStylePainter p(this);
  QStyleOptionFrame opt;
  this->initStyleOption(&opt);
  p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
  p.end();

  QLabel::paintEvent(ev);
}

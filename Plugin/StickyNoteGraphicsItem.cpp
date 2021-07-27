#include "StickyNoteGraphicsItem.h"

#include <pqPipelineSource.h>

#include <vtkSMProxy.h>
#include <vtkSMPropertyHelper.h>

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>

namespace ParaViewNetworkEditor {

StickyNoteGraphicsItem::StickyNoteGraphicsItem(pqPipelineSource* source) {
  this->source_ = source;
  setZValue(STICKYNOTEGRAPHICSITEM_DEPTH);
}

StickyNoteGraphicsItem::~StickyNoteGraphicsItem() = default;

void StickyNoteGraphicsItem::paint(QPainter *p, const QStyleOptionGraphicsItem *options, QWidget *widget) {
  if (!source_)
    return;
  bool modified = source_->modifiedState() != pqProxy::UNMODIFIED;
  p->save();
  p->setRenderHint(QPainter::Antialiasing, true);

  p->fillRect(this->rect(), QColor(255, 255, 255, 128));

  p->restore();
  p->save();

  QColor backgroundColor("#3b3d3d");
  if (auto proxy = source_->getProxy()) {
    double r = vtkSMPropertyHelper(proxy, "BackgroundColor").GetAsDouble(0);
    double g = vtkSMPropertyHelper(proxy, "BackgroundColor").GetAsDouble(1);
    double b = vtkSMPropertyHelper(proxy, "BackgroundColor").GetAsDouble(2);
    backgroundColor.setRgbF(r, g, b);
  }

  QRectF rect = this->rect();
  rect.adjust(0, 25, 0, 0);
  p->fillRect(rect, backgroundColor);

  QColor borderColor("#282828");
  if (modified) {
    borderColor = QColor("#FBBC05");
  } else if (isSelected()) {
    borderColor = QColor("#dd0308");
  }
  p->setBrush(QColor(0,0,0,0));
  p->setPen(QPen(QBrush(borderColor), 1.0));
  if (modified || isSelected()) {
    p->drawRect(this->rect());
  }
  p->restore();
}

void StickyNoteGraphicsItem::showToolTip(QGraphicsSceneHelpEvent *e) {
  if (!source_)
    return;
  QString caption, text;
  if (auto proxy = source_->getProxy()) {
    caption = vtkSMPropertyHelper(proxy, "Caption").GetAsString();
    text = vtkSMPropertyHelper(proxy, "Text").GetAsString();
  }
  QString s;
  s += "<html><body><table style=\"white-space: nowrap;\">";
  s += "<tr><td>" + caption + "</td></tr>";
  s += "<tr><td>" + text + "</td></tr>";
  s += "</table></body></html>";
  this->showToolTipHelper(e, s);
}


}

#include "SourceGraphicsItem.h"
#include <pqPipelineSource.h>
#include <QPainter>

const QSizeF SourceGraphicsItem::size_ = {150.f, 50.f};

SourceGraphicsItem::SourceGraphicsItem(pqPipelineSource *source)
: source_(source)
{
  static constexpr int labelHeight = 8;
  auto width = static_cast<int>(size_.width());

  setZValue(SOURCEGRAPHICSITEM_DEPTH);
  setFlags(ItemIsMovable | ItemIsSelectable | ItemIsFocusable | ItemSendsGeometryChanges);
  setRect(-size_.width() / 2, -size_.height() / 2, size_.width(), size_.height());

}

SourceGraphicsItem::~SourceGraphicsItem() = default;

void SourceGraphicsItem::paint(QPainter *p, const QStyleOptionGraphicsItem *options, QWidget *widget) {
  const float roundedCorners = 9.0f;

  p->save();
  p->setRenderHint(QPainter::Antialiasing, true);
  QColor selectionColor("#7a191b");
  QColor backgroundColor("#3b3d3d");
  QColor borderColor("#282828");

  if (isSelected()) {
    p->setBrush(selectionColor);
  } else {
    p->setBrush(backgroundColor);
  }
  p->setPen(QPen(QBrush(borderColor), 2.0));

  p->drawRoundedRect(rect(), roundedCorners, roundedCorners);

  p->restore();
}

void SourceGraphicsItem::editDisplayName() {

}

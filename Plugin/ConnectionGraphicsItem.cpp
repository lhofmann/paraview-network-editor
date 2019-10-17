#include "ConnectionGraphicsItem.h"

#include <QGraphicsDropShadowEffect>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QPainterPath>

#include <cmath>

const QColor dummy_color(44, 123, 182);

CurveGraphicsItem::CurveGraphicsItem(QColor color, QColor borderColor, QColor selectedBorderColor)
    : color_(color), borderColor_(borderColor), selectedBorderColor_(selectedBorderColor) {
  setZValue(DRAGING_ITEM_DEPTH);
}

CurveGraphicsItem::~CurveGraphicsItem() = default;

QPainterPath CurveGraphicsItem::obtainCurvePath() const {
  return obtainCurvePath(getStartPoint(), getEndPoint());
}

QPainterPath CurveGraphicsItem::obtainCurvePath(QPointF startPoint, QPointF endPoint) const {
  const int startOff = 6;

  QPointF curvStart = startPoint + QPointF(0, startOff);
  QPointF curvEnd = endPoint - QPointF(0, startOff);

  double delta = std::abs(curvEnd.y() - curvStart.y());

  QPointF o = curvEnd - curvStart;
  double min = 37 - startOff * 2;
  min = std::min(min, std::sqrt(o.x() * o.x() + o.y() * o.y()));
  static const double max = 40.0;
  if (delta < min) delta = min;
  if (delta > max) delta = max;

  QPointF off(0, delta);
  QPointF ctrlPoint1 = curvStart + off;
  QPointF ctrlPoint2 = curvEnd - off;

  QPainterPath bezierCurve;
  bezierCurve.moveTo(startPoint);
  bezierCurve.lineTo(curvStart);
  bezierCurve.cubicTo(ctrlPoint1, ctrlPoint2, curvEnd);
  bezierCurve.lineTo(endPoint);

  return bezierCurve;
}

void CurveGraphicsItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) {
  const auto color = getColor();
  if (isSelected()) {
    p->setPen(QPen(selectedBorderColor_, 4.0, Qt::SolidLine, Qt::RoundCap));
  } else {
    p->setPen(QPen(borderColor_, 3.0, Qt::SolidLine, Qt::RoundCap));
  }
  p->drawPath(path_);
  p->setPen(QPen(color, 2.0, Qt::SolidLine, Qt::RoundCap));
  p->drawPath(path_);
}

QPainterPath CurveGraphicsItem::shape() const {
  QPainterPathStroker pathStrocker;
  pathStrocker.setWidth(10.0);
  return pathStrocker.createStroke(path_);
}

void CurveGraphicsItem::resetBorderColors() {
  setBorderColor(Qt::black);
  setSelectedBorderColor(Qt::darkRed);
  update();
}

void CurveGraphicsItem::updateShape() {
  path_ = obtainCurvePath();
  const auto p = path_.boundingRect();
  rect_ = QRectF(p.topLeft() - QPointF(5, 5), p.size() + QSizeF(10, 10));
  prepareGeometryChange();
}

QRectF CurveGraphicsItem::boundingRect() const { return rect_; }

void CurveGraphicsItem::setColor(QColor color) { color_ = color; }

void CurveGraphicsItem::setBorderColor(QColor borderColor) { borderColor_ = borderColor; }

void CurveGraphicsItem::setSelectedBorderColor(QColor selectedBorderColor) {
  selectedBorderColor_ = selectedBorderColor;
}

QColor CurveGraphicsItem::getColor() const { return color_; }


ConnectionGraphicsItem::ConnectionGraphicsItem(OutputPortGraphicsItem* outport,
                                               InputPortGraphicsItem* inport)
    : CurveGraphicsItem(dummy_color) // (utilqt::toQColor(connection.getInport()->getColorCode()))
    , outport_(outport)
    , inport_(inport)
{
  setFlags(ItemIsSelectable | ItemIsFocusable);
  setZValue(CONNECTIONGRAPHICSITEM_DEPTH);
  outport_->addConnection(this);
  inport_->addConnection(this);
}

ConnectionGraphicsItem::~ConnectionGraphicsItem() {
  outport_->removeConnection(this);
  inport_->removeConnection(this);
}

QPointF ConnectionGraphicsItem::getStartPoint() const {
  return outport_->mapToScene(outport_->rect().center());
}

QPointF ConnectionGraphicsItem::getEndPoint() const {
  return inport_->mapToScene(inport_->rect().center());
}

void ConnectionGraphicsItem::showToolTip(QGraphicsSceneHelpEvent* e) {
  // showPortInfo(e, getOutport());
}

QVariant ConnectionGraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value) {
  switch (change) {
    case QGraphicsItem::ItemSelectedHasChanged:
      inport_->update();
      outport_->update();
      break;
    default:
      break;
  }
  return QGraphicsItem::itemChange(change, value);
}

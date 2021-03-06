#include "ConnectionGraphicsItem.h"
#include "utilpq.h"
#include "NetworkEditor.h"

#include <QGraphicsDropShadowEffect>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>

#include <cmath>

namespace ParaViewNetworkEditor {

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

void CurveGraphicsItem::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) {
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

ConnectionDragGraphicsItem::ConnectionDragGraphicsItem(OutputPortGraphicsItem *outport,
                                                       QPointF endPoint, QColor color)
    : CurveGraphicsItem(color), endPoint_{endPoint}, outport_(outport) {}

ConnectionDragGraphicsItem::~ConnectionDragGraphicsItem() = default;

OutputPortGraphicsItem *ConnectionDragGraphicsItem::getOutportGraphicsItem() const {
  return outport_;
}

QPointF ConnectionDragGraphicsItem::getStartPoint() const {
  return outport_->mapToScene(outport_->rect().center());
}

QPointF ConnectionDragGraphicsItem::getEndPoint() const { return endPoint_; }

void ConnectionDragGraphicsItem::setEndPoint(QPointF endPoint) {
  endPoint_ = endPoint;
  updateShape();
}

void ConnectionDragGraphicsItem::reactToPortHover(InputPortGraphicsItem *inport) {
  if (inport != nullptr) {
    auto ip = inport->getPort();
    auto op = outport_->getPort();

    bool force_accept = QApplication::keyboardModifiers() & Qt::ShiftModifier;
    if (force_accept || utilpq::can_connect(op.first, op.second, ip.first, ip.second)) {
      setBorderColor(Qt::green);
    } else {
      setBorderColor(Qt::red);
    }
  } else {
    resetBorderColors();
  }
}

QColor ConnectionDragGraphicsItem::getColor() const {
  if (!outport_)
    return color_;
  auto source_port = outport_->getPort();
  return utilpq::output_dataset_color(std::get<0>(source_port), std::get<1>(source_port));
}

ConnectionGraphicsItem::ConnectionGraphicsItem(OutputPortGraphicsItem *outport,
                                               InputPortGraphicsItem *inport)
    : CurveGraphicsItem(utilpq::default_color)
    , outport_(outport), inport_(inport) {
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

void ConnectionGraphicsItem::showToolTip(QGraphicsSceneHelpEvent *e) {
  auto inport = this->outport_->getPort();
  auto outport = this->inport_->getPort();
  this->showConnectionInfo(e, std::get<0>(inport), std::get<1>(inport),
                           std::get<0>(outport), std::get<1>(outport));
}

QVariant ConnectionGraphicsItem::itemChange(GraphicsItemChange change, const QVariant &value) {
  switch (change) {
    case QGraphicsItem::ItemSelectedHasChanged:inport_->update();
      outport_->update();
      break;
    default:break;
  }
  return QGraphicsItem::itemChange(change, value);
}

InputPortGraphicsItem *ConnectionGraphicsItem::getInportGraphicsItem() const {
  return inport_;
}

OutputPortGraphicsItem *ConnectionGraphicsItem::getOutportGraphicsItem() const {
  return outport_;
}

QColor ConnectionGraphicsItem::getColor() const {
  if (!outport_)
    return color_;
  auto source_port = outport_->getPort();
  return utilpq::output_dataset_color(std::get<0>(source_port), std::get<1>(source_port));
}

void ConnectionGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *e) {
  if (e->buttons() == Qt::LeftButton) {
    if (this->outport_)
      getNetworkEditor()->initiateConnection(this->outport_);
  }
  e->accept();
}


}

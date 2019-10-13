#include "PortGraphicsItem.h"

#include "SourceGraphicsItem.h"

#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>

#include <QPen>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsView>
#include <QToolTip>

#include <iostream>

PortGraphicsItem::PortGraphicsItem(SourceGraphicsItem* parent, const QPointF& pos, bool up, QColor color)
: EditorGraphicsItem(parent), source_(parent), size_(9.0f), lineWidth_(1.0f)
{
  setRect(-(0.5f * size_ + lineWidth_), -(0.5f * size_ + lineWidth_), size_ + 2.0 * lineWidth_,
          size_ + 2.0 * lineWidth_);
  setPos(pos);
  setFlags(ItemSendsScenePositionChanges);

  connectionIndicator_ = new PortConnectionIndicator(this, up, color);
  connectionIndicator_->setVisible(false);
}

/*
void PortGraphicsItem::addConnection(ConnectionGraphicsItem* connection) {
  connections_.push_back(connection);
  connectionIndicator_->setVisible(true);
  updateConnectionPositions();
  update();  // we need to repaint the connection
}

void PortGraphicsItem::removeConnection(ConnectionGraphicsItem* connection) {
  connections_.erase(std::find(connections_.begin(), connections_.end(), connection));
  connectionIndicator_->setVisible(!connections_.empty());
  update();  // we need to repaint the connection
}
 */

QVariant PortGraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value) {
  if (change == QGraphicsItem::ItemScenePositionHasChanged) {
    updateConnectionPositions();
  }
  return EditorGraphicsItem::itemChange(change, value);
}

/*
std::vector<ConnectionGraphicsItem*>& PortGraphicsItem::getConnections() {
  return connections_;
}
*/

SourceGraphicsItem* PortGraphicsItem::getSource() { return source_; }

PortGraphicsItem::~PortGraphicsItem() = default;

InputPortGraphicsItem::InputPortGraphicsItem(SourceGraphicsItem* parent, const QPointF& pos, pqPipelineFilter* source, int port_id)
: PortGraphicsItem(parent, pos, true, QColor(25, 25, 255))
{}

void InputPortGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent* e) {
  /*
  if (e->buttons() == Qt::LeftButton && inport_->isConnected()) {
    getNetworkEditor()->releaseConnection(this);
  }
  e->accept();
   */
}

void InputPortGraphicsItem::updateConnectionPositions() {
  /*
  for (auto& elem : connections_) {
    elem->updateShape();
  }
   */
}

void InputPortGraphicsItem::showToolTip(QGraphicsSceneHelpEvent* e) {
  // showPortInfo(e, inport_);
}

void InputPortGraphicsItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) {
  p->save();
  p->setRenderHint(QPainter::Antialiasing, true);
  p->setRenderHint(QPainter::SmoothPixmapTransform, true);

  QColor borderColor(40, 40, 40);

  // uvec3 color = inport_->getColorCode();
  QColor color(255, 25, 25);

  QRectF portRect(QPointF(-size_, size_) / 2.0f, QPointF(size_, -size_) / 2.0f);
  p->setBrush(color);
  p->setPen(QPen(borderColor, lineWidth_));

  if (true /*inport_->isOptional()*/) {
    // Use a different shape for optional ports (rounded at the bottom)
    QPainterPath path;
    auto start = (portRect.topRight() + portRect.bottomRight()) * 0.5;
    path.moveTo(start);
    path.lineTo(portRect.bottomRight());
    path.lineTo(portRect.bottomLeft());
    path.lineTo((portRect.topLeft() + portRect.bottomLeft()) * 0.5);
    path.arcTo(portRect, 180, -180);
    p->drawPath(path);

    // Draw a dot in the middle
    const qreal radius = 1.5;
    QRectF dotRect(QPointF(-radius, radius), QPointF(radius, -radius));
    p->setPen(borderColor.lighter(180));
    p->setBrush(borderColor.lighter(180));
    p->drawEllipse(dotRect);

  } else {
    p->drawRect(portRect);
  }

  p->restore();
}

OutputPortGraphicsItem::OutputPortGraphicsItem(SourceGraphicsItem* parent,  const QPointF& pos, pqPipelineSource* source, int port_id)
: PortGraphicsItem(parent, pos, false, QColor(25,255,25))
{}

void OutputPortGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent* e) {
  /*
  if (e->buttons() == Qt::LeftButton) {
    getNetworkEditor()->initiateConnection(this);
  }
  e->accept();
   */
}

void OutputPortGraphicsItem::updateConnectionPositions() {
  /*
  for (auto& elem : connections_) {
    elem->updateShape();
  }
   */
}

void OutputPortGraphicsItem::showToolTip(QGraphicsSceneHelpEvent* e) {
  showToolTipHelper(e, "Hello <b>World</b>!");
}

void OutputPortGraphicsItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) {
  p->save();
  p->setRenderHint(QPainter::Antialiasing, true);
  p->setRenderHint(QPainter::SmoothPixmapTransform, true);

  QColor borderColor(40, 40, 40);
  // uvec3 color = outport_->getColorCode();
  QColor color(25,25,255);

  QRectF portRect(QPointF(-size_, size_) / 2.0f, QPointF(size_, -size_) / 2.0f);
  p->setBrush(color);
  p->setPen(QPen(borderColor, lineWidth_));
  p->drawRect(portRect);
  p->restore();
}

PortConnectionIndicator::PortConnectionIndicator(
    PortGraphicsItem* parent, bool up, QColor color)
    : EditorGraphicsItem(parent), portConnectionItem_(parent), up_(up), color_(color) {
  setRect(-2.0f, -8.0f, 4.0f, 16.0f);
  setPos(0.0f, 0.0f);
}

void PortConnectionIndicator::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) {
  p->save();
  p->setRenderHint(QPainter::Antialiasing, true);

  /*
  const bool selected = util::any_of(portConnectionItem_->getConnections(),
                                     [](auto& connection) { return connection->isSelected(); });
                                     */
  const bool selected = true;

  const float width{selected ? (4.0f - 1.0f) / 2.0f : (3.0f - 0.5f) / 2.0f};
  const float length = 7.0f;

  QPainterPath path;
  if (up_) {
    path.moveTo(-width, -length);
    path.lineTo(-width, 0.0f);
    path.arcTo(QRectF(-width, -width, 2 * width, 2 * width), 180.0f, 180.0f);
    path.lineTo(width, -length);
  } else {
    path.moveTo(-width, length);
    path.lineTo(-width, 0.0f);
    path.arcTo(QRectF(-width, -width, 2 * width, 2 * width), 180.0f, -180.0f);
    path.lineTo(width, length);
  }
  QPainterPath closedPath(path);
  closedPath.closeSubpath();

  QLinearGradient gradBrush(QPointF(0.0f, 0.0f), QPointF(0.0, up_ ? -length : length));
  gradBrush.setColorAt(0.0f, color_);
  gradBrush.setColorAt(0.75f, color_);
  gradBrush.setColorAt(1.0f, QColor(color_.red(), color_.green(), color_.blue(), 0));

  QLinearGradient gradPen(QPointF(0.0f, 0.0f), QPointF(0.0, up_ ? -length : length));
  QColor lineColor = selected ? Qt::darkRed : Qt::black;
  gradPen.setColorAt(0.0f, lineColor);
  gradPen.setColorAt(0.75f, lineColor);
  gradPen.setColorAt(1.0f, QColor(lineColor.red(), lineColor.green(), lineColor.blue(), 0));

  p->setPen(Qt::NoPen);
  p->setBrush(color_);
  p->drawPath(closedPath);

  p->setBrush(Qt::NoBrush);
  p->setPen(QPen(lineColor, selected ? 1.0f : 0.5f));
  p->drawPath(path);

  p->restore();
}

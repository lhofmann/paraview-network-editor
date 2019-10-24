#include "ConnectionDragHelper.h"
#include "NetworkEditor.h"
#include "PortGraphicsItem.h"
#include "ConnectionGraphicsItem.h"
#include "utilpq.h"

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>

ConnectionDragHelper::ConnectionDragHelper(NetworkEditor &editor)
    : QObject(&editor), editor_{editor} {}

ConnectionDragHelper::~ConnectionDragHelper() = default;

bool ConnectionDragHelper::eventFilter(QObject *, QEvent *event) {
  if (connection_ && event->type() == QEvent::GraphicsSceneMouseMove) {
    auto e = static_cast<QGraphicsSceneMouseEvent *>(event);
    connection_->setEndPoint(e->scenePos());
    connection_->reactToPortHover(editor_.getInputPortGraphicsItemAt(e->scenePos()));
    e->accept();
  } else if (connection_ && event->type() == QEvent::GraphicsSceneMouseRelease) {
    auto e = static_cast<QGraphicsSceneMouseEvent *>(event);

    auto endItem = editor_.getInputPortGraphicsItemAt(e->scenePos());
    auto outport = connection_->getOutportGraphicsItem()->getPort();
    reset();

    if (endItem) {
      auto inport = endItem->getPort();
      if (utilpq::can_connect(std::get<0>(outport), std::get<1>(outport), std::get<0>(inport), std::get<1>(inport))) {
        if (!utilpq::multiple_inputs(std::get<0>(inport), std::get<1>(inport))) {
          utilpq::clear_connections(std::get<0>(inport), std::get<1>(inport));
        }
        utilpq::add_connection(std::get<0>(outport), std::get<1>(outport), std::get<0>(inport), std::get<1>(inport));
      }
    }
    e->accept();
  }
  return false;
}

void ConnectionDragHelper::start(OutputPortGraphicsItem *outport, QPointF endPoint, QColor color) {
  connection_ =
      std::make_unique<ConnectionDragGraphicsItem>(outport, endPoint, color);
  editor_.addItem(connection_.get());
  connection_->show();
}

void ConnectionDragHelper::reset() { connection_.reset(); }

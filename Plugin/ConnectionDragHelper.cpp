#include "ConnectionDragHelper.h"
#include "NetworkEditor.h"
#include "PortGraphicsItem.h"
#include "ConnectionGraphicsItem.h"
#include "utilpq.h"

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QApplication>

namespace ParaViewNetworkEditor {

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
      bool force_accept = QApplication::keyboardModifiers() & Qt::ShiftModifier;
      if (force_accept || utilpq::can_connect(outport.first, outport.second, inport.first, inport.second)) {
        if (!utilpq::multiple_inputs(inport.first, inport.second)) {
          utilpq::clear_connections(inport.first, inport.second);
        }
        utilpq::add_connection(outport.first, outport.second, inport.first, inport.second);
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

}

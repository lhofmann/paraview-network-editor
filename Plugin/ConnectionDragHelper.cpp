#include "ConnectionDragHelper.h"
#include "NetworkEditor.h"
#include "PortGraphicsItem.h"
#include "ConnectionGraphicsItem.h"

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

    // auto startPort = connection_->getOutportGraphicsItem()->getPort();
    reset();

    auto endItem = editor_.getInputPortGraphicsItemAt(e->scenePos());
    /*
    if (endItem && endItem->getPort()->canConnectTo(startPort)) {
      Inport *endPort = endItem->getPort();

      if (endPort->getNumberOfConnections() >= endPort->getMaxNumberOfConnections()) {
        editor_.getNetwork()->removeConnection(endPort->getConnectedOutport(), endPort);
      }
      editor_.getNetwork()->addConnection(startPort, endPort);
    }
    */
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

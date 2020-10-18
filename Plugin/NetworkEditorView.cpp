#include "NetworkEditorView.h"
#include "NetworkEditor.h"
#include "utilqt.h"

#include <QWheelEvent>
#include <QtMath>
#include <QScrollBar>

#include <vtkLogger.h>

namespace ParaViewNetworkEditor {

NetworkEditorView::NetworkEditorView(NetworkEditor *networkEditor, QWidget *parent)
    : QGraphicsView(parent),
      editor_(networkEditor) {
  QGraphicsView::setScene(editor_);
  setRenderHint(QPainter::Antialiasing, true);
  setMouseTracking(true);
  setDragMode(QGraphicsView::RubberBandDrag);
  setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
  setCacheMode(QGraphicsView::CacheBackground);

  const auto scale = utilqt::emToPx(this, 1.0) / static_cast<double>(utilqt::refEm());
  setTransform(QTransform::fromScale(scale, scale), false);

  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

}

NetworkEditorView::~NetworkEditorView() {
  QGraphicsView::setScene(nullptr);
}

bool NetworkEditorView::viewportEvent(QEvent *event) {
  editor_->storeTransform(this->transform(), this->verticalScrollBar()->value(), this->horizontalScrollBar()->value());
  return QGraphicsView::viewportEvent(event);
}

void NetworkEditorView::keyPressEvent(QKeyEvent *keyEvent) {
  if (keyEvent->modifiers() & Qt::ControlModifier) {
    setDragMode(QGraphicsView::ScrollHandDrag);
  }
  QGraphicsView::keyPressEvent(keyEvent);
}

void NetworkEditorView::keyReleaseEvent(QKeyEvent *keyEvent) {
  setDragMode(QGraphicsView::RubberBandDrag);
  QGraphicsView::keyReleaseEvent(keyEvent);
}

void NetworkEditorView::focusOutEvent(QFocusEvent *e) {
  setDragMode(QGraphicsView::RubberBandDrag);
  QGraphicsView::focusOutEvent(e);
}

void NetworkEditorView::wheelEvent(QWheelEvent *e) {
  QPointF numPixels = e->pixelDelta() / 5.0;
  QPointF numDegrees = e->angleDelta() / 8.0 / 15;

  if (e->modifiers() == Qt::ControlModifier) {
    if (!numPixels.isNull()) {
      zoom(qPow(1.025, std::max(-15.0, std::min(15.0, numPixels.y()))));
    } else if (!numDegrees.isNull()) {
      zoom(qPow(1.025, std::max(-15.0, std::min(15.0, numDegrees.y()))));
    }
  } else if (e->modifiers() & Qt::ShiftModifier) {
    // horizontal scrolling
    auto modifiers = e->modifiers();
    // remove the shift key temporarily from the event
    e->setModifiers(e->modifiers() ^ Qt::ShiftModifier);
    horizontalScrollBar()->event(e);
    // restore previous modifiers
    e->setModifiers(modifiers);
  } else {
    QGraphicsView::wheelEvent(e);
  }
  e->accept();
}

void NetworkEditorView::zoom(double dz) {
  if ((dz > 1.0 && transform().m11() > 8.0) || (dz < 1.0 && transform().m11() < 0.125)) return;

  setTransform(QTransform::fromScale(dz, dz), true);
}

void NetworkEditorView::mouseDoubleClickEvent(QMouseEvent *e) {
  QGraphicsView::mouseDoubleClickEvent(e);

  if (!e->isAccepted()) {
    fitNetwork();
    e->accept();
  }
}

void NetworkEditorView::fitNetwork() {
  const auto scale = utilqt::emToPx(this, 1.0) / static_cast<double>(utilqt::refEm());
  setTransform(QTransform::fromScale(scale, scale), false);
  if (!editor_->empty()) {
    const auto br = editor_->getSourcesBoundingRect().adjusted(-50, -50, 50, 50);
    editor_->setSceneRect(br);
    fitInView(br, Qt::KeepAspectRatio);
  } else {
    QRectF r{rect()};
    r.moveCenter(QPointF(0, 0));
    editor_->setSceneRect(rect());
    fitInView(rect(), Qt::KeepAspectRatio);
  }
  if (transform().m11() > scale) {
    setTransform(QTransform::fromScale(scale, scale), false);
  }
}

}

#include "NetworkEditorView.h"
#include "NetworkEditor.h"
#include "utilqt.h"

#include <QWheelEvent>
#include <QtMath>
#include <QScrollBar>

NetworkEditorView::NetworkEditorView(NetworkEditor *networkEditor, QWidget *parent)
: QGraphicsView(parent),
  editor_(networkEditor)
{
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

void NetworkEditorView::wheelEvent(QWheelEvent* e) {
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
  if ((dz > 1.0 && matrix().m11() > 8.0) || (dz < 1.0 && matrix().m11() < 0.125)) return;

  setTransform(QTransform::fromScale(dz, dz), true);
}

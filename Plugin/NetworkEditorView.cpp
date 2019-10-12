#include "NetworkEditorView.h"
#include "NetworkEditor.h"
#include "utilqt.h"

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

#include "NetworkEditor.h"
#include <QPainter>

const int NetworkEditor::gridSpacing_ = 25;

NetworkEditor::NetworkEditor() {
  // The default BSP tends to crash...
  setItemIndexMethod(QGraphicsScene::NoIndex);
  setSceneRect(QRectF());
}

NetworkEditor::~NetworkEditor() = default;

void NetworkEditor::drawBackground(QPainter* painter, const QRectF& rect) {
  painter->save();
  painter->setWorldMatrixEnabled(true);
  painter->fillRect(rect, QColor(0x7d, 0x80, 0x83));
  qreal left = int(rect.left()) - (int(rect.left()) % gridSpacing_);
  qreal top = int(rect.top()) - (int(rect.top()) % gridSpacing_);
  QVarLengthArray<QLineF, 100> linesX;
  painter->setPen(QColor(153, 153, 153));

  for (qreal x = left; x < rect.right(); x += gridSpacing_)
    linesX.append(QLineF(x, rect.top(), x, rect.bottom()));

  QVarLengthArray<QLineF, 100> linesY;

  for (qreal y = top; y < rect.bottom(); y += gridSpacing_)
    linesY.append(QLineF(rect.left(), y, rect.right(), y));

  painter->drawLines(linesX.data(), linesX.size());
  painter->drawLines(linesY.data(), linesY.size());
  painter->restore();
}

void NetworkEditor::drawForeground(QPainter* painter, const QRectF& rect) {
  // For testing purposes only. Draw bounding rects around all graphics items

  QList<QGraphicsItem*> items = QGraphicsScene::items(Qt::DescendingOrder);
  painter->setPen(Qt::magenta);
  for (QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); ++it) {
      QRectF br = (*it)->sceneBoundingRect();
      painter->drawRect(br);
  }
  painter->setPen(Qt::red);
  painter->drawRect(QGraphicsScene::itemsBoundingRect());
}

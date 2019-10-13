#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITOR_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITOR_H_

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <map>

class SourceGraphicsItem;
class pqPipelineSource;

class NetworkEditor : public QGraphicsScene {
  Q_OBJECT
 public:
  NetworkEditor();
  ~NetworkEditor() override;

  void addSourceRepresentation(pqPipelineSource* source);

 private:
  // Get QGraphicsItems
  template <typename T>
  T* getGraphicsItemAt(const QPointF pos) const;

  void drawBackground(QPainter* painter, const QRectF& rect) override;
  void drawForeground(QPainter* painter, const QRectF& rect) override;

  std::map<pqPipelineSource*, SourceGraphicsItem*> sourceGraphicsItems_;

  static const int gridSpacing_;
};

template <typename T>
T* NetworkEditor::getGraphicsItemAt(const QPointF pos) const {
  QList<QGraphicsItem*> graphicsItems = items(pos);
  for (int i = 0; i < graphicsItems.size(); i++) {
    if (auto item = qgraphicsitem_cast<T*>(graphicsItems[i])) return item;
  }
  return nullptr;
}

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITOR_H_

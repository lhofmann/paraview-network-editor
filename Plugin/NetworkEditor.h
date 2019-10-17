#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITOR_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITOR_H_

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <map>
#include <tuple>

class SourceGraphicsItem;
class ConnectionGraphicsItem;
class pqPipelineSource;
class QGraphicsSceneContextMenuEvent;

class NetworkEditor : public QGraphicsScene {
  Q_OBJECT
 public:
  NetworkEditor();
  ~NetworkEditor() override;

  void addSourceRepresentation(pqPipelineSource* source);
  void removeSourceRepresentation(pqPipelineSource* source);
  void updateConnectionRepresentations(pqPipelineSource* source, pqPipelineSource* dest);
  void setAutoUpdateActiveObject(bool);

 protected:
  virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* e) override;
  void onSelectionChanged();

  void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;

  void helpEvent(QGraphicsSceneHelpEvent* helpEvent) override;

 private:
  // Get QGraphicsItems
  template <typename T>
  T* getGraphicsItemAt(const QPointF pos) const;

  void drawBackground(QPainter* painter, const QRectF& rect) override;
  void drawForeground(QPainter* painter, const QRectF& rect) override;

  std::map<pqPipelineSource*, SourceGraphicsItem*> sourceGraphicsItems_;

  std::map<std::tuple<pqPipelineSource*, int, pqPipelineSource*, int>, ConnectionGraphicsItem*> connectionGraphicsItems_;

  bool autoUpdateActiveObject_ = false;
  bool updateSelection_ = false;

  static const int gridSpacing_;
  QPointF snapToGrid(const QPointF& pos);
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

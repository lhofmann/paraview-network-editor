#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITOR_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITOR_H_

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <map>
#include <tuple>

class pqPipelineSource;
class QGraphicsSceneContextMenuEvent;
class pqDeleteReaction;

namespace ParaViewNetworkEditor {

class SourceGraphicsItem;
class ConnectionGraphicsItem;
class ConnectionDragHelper;
class OutputPortGraphicsItem;
class InputPortGraphicsItem;

class NetworkEditor : public QGraphicsScene {
 Q_OBJECT
 public:
  NetworkEditor();
  ~NetworkEditor() override;

  void addSourceRepresentation(pqPipelineSource *source);
  void removeSourceRepresentation(pqPipelineSource *source);
  void updateConnectionRepresentations(pqPipelineSource *source, pqPipelineSource *dest);

  void removeConnection(ConnectionGraphicsItem *);

  void showSelected();
  void hideSelected();
  void showSelectedScalarBars();
  void hideSelectedScalarBars();

  void selectAll();
  void deleteSelected();
  void copy();
  void paste(float x, float y);
  void paste();
  void quickLaunch();

  void computeGraphLayout();

  void updateSourcePositions();
  void updateSceneSize();
  bool empty() const;
  QRectF getSourcesBoundingRect() const;

  // Called from ProcessorPortGraphicsItems mouse events.
  void initiateConnection(OutputPortGraphicsItem *item);
  void releaseConnection(InputPortGraphicsItem *item);

  InputPortGraphicsItem *getInputPortGraphicsItemAt(const QPointF pos) const;

 protected:
  virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *e) override;
  void onSelectionChanged();

  void mousePressEvent(QGraphicsSceneMouseEvent *) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent *) override;

  void helpEvent(QGraphicsSceneHelpEvent *helpEvent) override;

 private:
  // Get QGraphicsItems
  template <typename T>
  T *getGraphicsItemAt(const QPointF pos) const;

  void drawBackground(QPainter *painter, const QRectF &rect) override;
  void drawForeground(QPainter *painter, const QRectF &rect) override;

  std::map<pqPipelineSource *, SourceGraphicsItem *> sourceGraphicsItems_;

  // connectionGraphicsItems_[source, dest][output_id, input_id]
  std::map<std::tuple<pqPipelineSource *, pqPipelineSource *>, std::map<std::tuple<int, int>, ConnectionGraphicsItem *>>
      connectionGraphicsItems_;

  bool selectionDirty_ = false;
  bool updateSelection_ = false;
  static const int gridSpacing_;
  QPointF snapToGrid(const QPointF &pos);

  QPointF lastMousePos_ = QPointF(0., 0.);
  bool addSourceAtMousePos_ = false;
  bool addSourceToSelection_ = false;

  SourceGraphicsItem *activeSourceItem_{nullptr};

  ConnectionDragHelper *connectionDragHelper_;
  pqDeleteReaction *deleteReaction_;
};

template <typename T>
T *NetworkEditor::getGraphicsItemAt(const QPointF pos) const {
  QList<QGraphicsItem *> graphicsItems = items(pos);
  for (int i = 0; i < graphicsItems.size(); i++) {
    if (auto item = qgraphicsitem_cast<T *>(graphicsItems[i])) return item;
  }
  return nullptr;
}

}

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITOR_H_

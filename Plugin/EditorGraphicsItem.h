#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_EDITORGRAPHICSITEM_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_EDITORGRAPHICSITEM_H_

#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneHelpEvent>

class pqPipelineSource;
class pqPipelineFilter;

namespace ParaViewNetworkEditor {

enum NetworkEditorGraphicsItemType {
  SourceGraphicsType = 1,
  CurveGraphicsType,
  ConnectionDragGraphicsType,
  ConnectionGraphicsType,
  LinkGraphicsType,
  LinkConnectionDragGraphicsType,
  LinkConnectionGraphicsType,
  SourceProgressGraphicsType,
  OutputPortStatusGraphicsType,
  SourceLinkGraphicsType,
  InputPortGraphicsType,
  OutputPortGraphicsType
};

// Z value for various graphics items.
static const qreal DRAGING_ITEM_DEPTH = 5.0f;
static const qreal SELECTED_SOURCEGRAPHICSITEM_DEPTH = 4.0f;
static const qreal SOURCEGRAPHICSITEM_DEPTH = 3.0f;
static const qreal CONNECTIONGRAPHICSITEM_DEPTH = 2.0f;
static const qreal LINKGRAPHICSITEM_DEPTH = 1.0f;
static const qreal STICKYNOTEGRAPHICSITEM_DEPTH = 0.0f;

class Port;
class NetworkEditor;

class EditorGraphicsItem : public QGraphicsRectItem {
 public:
  EditorGraphicsItem();
  EditorGraphicsItem(QGraphicsItem *parent);
  virtual ~EditorGraphicsItem();
  QPoint mapPosToSceen(QPointF pos) const;

  virtual void showToolTip(QGraphicsSceneHelpEvent *event);

  void showSourceInfo(QGraphicsSceneHelpEvent *event, pqPipelineSource* source) const;
  void showInportInfo(QGraphicsSceneHelpEvent *event, pqPipelineFilter* source, int port) const;
  void showOutportInfo(QGraphicsSceneHelpEvent *event, pqPipelineSource* source, int port) const;
  void showConnectionInfo(QGraphicsSceneHelpEvent *event,
                          pqPipelineSource* source, int output_port,
                          pqPipelineFilter* dest, int input_port) const;

 protected:
  void showToolTipHelper(QGraphicsSceneHelpEvent *event, QString string) const;
  NetworkEditor *getNetworkEditor() const;
};

}

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_EDITORGRAPHICSITEM_H_

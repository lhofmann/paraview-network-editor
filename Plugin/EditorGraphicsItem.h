#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_EDITORGRAPHICSITEM_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_EDITORGRAPHICSITEM_H_

#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneHelpEvent>

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
static const qreal DRAGING_ITEM_DEPTH = 4.0f;
static const qreal SELECTED_SOURCEGRAPHICSITEM_DEPTH = 3.0f;
static const qreal SOURCEGRAPHICSITEM_DEPTH = 2.0f;
static const qreal CONNECTIONGRAPHICSITEM_DEPTH = 1.0f;
static const qreal LINKGRAPHICSITEM_DEPTH = 0.0f;

class Port;

class EditorGraphicsItem : public QGraphicsRectItem {
 public:
  EditorGraphicsItem();
  EditorGraphicsItem(QGraphicsItem* parent);
  virtual ~EditorGraphicsItem();
  QPoint mapPosToSceen(QPointF pos) const;

  virtual void showToolTip(QGraphicsSceneHelpEvent* event);
 protected:
  void showToolTipHelper(QGraphicsSceneHelpEvent* event, QString string) const;
};

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_EDITORGRAPHICSITEM_H_

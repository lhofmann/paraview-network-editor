#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_OUTPUTPORTSTATUSGRAPHICSITEM_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_OUTPUTPORTSTATUSGRAPHICSITEM_H_

#include "EditorGraphicsItem.h"

#include <QEvent>
#include <QRectF>

class pqOutputPort;

namespace ParaViewNetworkEditor {

class OutputPortStatusGraphicsItem : public EditorGraphicsItem {
 public:
  OutputPortStatusGraphicsItem(QGraphicsRectItem *parent, int port);
  virtual ~OutputPortStatusGraphicsItem() = default;

  // override for qgraphicsitem_cast (refer qt documentation)
  enum { Type = UserType + OutputPortStatusGraphicsType };
  int type() const override { return Type; }

 protected:
  void paint(QPainter *p, const QStyleOptionGraphicsItem *options, QWidget *widget) override;
  void mousePressEvent(QGraphicsSceneMouseEvent *e) override;
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e) override;

 private:
  float size_;
  float lineWidth_;
  int portID_;
};

}

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_OUTPUTPORTSTATUSGRAPHICSITEM_H_

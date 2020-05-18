#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_CONNECTIONGRAPHICSITEM_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_CONNECTIONGRAPHICSITEM_H_

#include "EditorGraphicsItem.h"
#include "PortGraphicsItem.h"

#include <QGraphicsLineItem>
#include <QPainterPath>
#include <QEvent>
#include <QColor>
#include <QPointF>

namespace ParaViewNetworkEditor {

class CurveGraphicsItem : public EditorGraphicsItem {
 public:
  CurveGraphicsItem(QColor color = QColor(38, 38, 38), QColor borderColor = Qt::black,
                    QColor selectedBorderColor = Qt::darkRed);
  virtual ~CurveGraphicsItem();

  virtual QPointF getStartPoint() const = 0;
  virtual QPointF getEndPoint() const = 0;

  virtual QPainterPath shape() const override;
  virtual QColor getColor() const;

  virtual void updateShape();

  virtual void setColor(QColor color);

  void resetBorderColors();
  void setBorderColor(QColor borderColor);
  void setSelectedBorderColor(QColor selectedBorderColor);

  // override for qgraphicsitem_cast (refer qt documentation)
  enum { Type = UserType + CurveGraphicsType };
  virtual int type() const override { return Type; }

  /**
   * Overloaded paint method from QGraphicsItem. Here the actual representation is drawn.
   */
  virtual void paint(QPainter *p, const QStyleOptionGraphicsItem *options,
                     QWidget *widget) override;
  virtual QRectF boundingRect() const override;

  virtual QPainterPath obtainCurvePath() const;
  virtual QPainterPath obtainCurvePath(QPointF startPoint, QPointF endPoint) const;

 protected:
  QColor color_;
  QColor borderColor_;
  QColor selectedBorderColor_;

  QPainterPath path_;
  QRectF rect_;
};

class ConnectionDragGraphicsItem : public CurveGraphicsItem {
 public:
  ConnectionDragGraphicsItem(OutputPortGraphicsItem *outport, QPointF endPoint,
                             QColor color = QColor(38, 38, 38));
  virtual ~ConnectionDragGraphicsItem();

// Override
  virtual QPointF getStartPoint() const override;
  virtual QPointF getEndPoint() const override;
  void setEndPoint(QPointF endPoint);

  OutputPortGraphicsItem *getOutportGraphicsItem() const;

  void reactToPortHover(InputPortGraphicsItem *inport);

// override for qgraphicsitem_cast (refer qt documentation)
  enum { Type = UserType + ConnectionDragGraphicsType };
  virtual int type() const override { return Type; }

 protected:
  QPointF endPoint_;
  OutputPortGraphicsItem *outport_;
};

class ConnectionGraphicsItem : public CurveGraphicsItem {
 public:
  ConnectionGraphicsItem(OutputPortGraphicsItem *outport,
                         InputPortGraphicsItem *inport);

  ~ConnectionGraphicsItem();

  // override for qgraphicsitem_cast (refer qt documentation)
  enum { Type = UserType + ConnectionGraphicsType };
  virtual int type() const override { return Type; }

  // Override
  virtual QPointF getStartPoint() const override;
  virtual QPointF getEndPoint() const override;

  InputPortGraphicsItem *getInportGraphicsItem() const;
  OutputPortGraphicsItem *getOutportGraphicsItem() const;

  virtual void showToolTip(QGraphicsSceneHelpEvent *e) override;

  virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

 private:
  OutputPortGraphicsItem *outport_;
  InputPortGraphicsItem *inport_;
};

}

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_CONNECTIONGRAPHICSITEM_H_

#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_STICKYNOTEGRAPHICSITEM_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_STICKYNOTEGRAPHICSITEM_H_

#include "SourceGraphicsItem.h"

namespace ParaViewNetworkEditor {

class StickyNoteGraphicsItem : public SourceGraphicsItem {
 Q_OBJECT
 public:
  StickyNoteGraphicsItem(pqPipelineSource* source);
  virtual ~StickyNoteGraphicsItem();

  void showToolTip(QGraphicsSceneHelpEvent *e) override;
  void storePosition() override;
  void loadPosition() override;
 protected:
  void paint(QPainter *p, const QStyleOptionGraphicsItem *options, QWidget *widget) override;

  enum Handle {
    HANDLE_BOTTOM = 0,
    HANDLE_RIGHT = 1,
    HANDLE_BOTTOM_RIGHT = 2,
    HANDLE_NONE = 3
  };
  Handle handle_selected_ {HANDLE_NONE};
  QPointF mouse_press_pos_;
  QRectF mouse_press_rect_;
  QRectF handles_[3];
  static const QCursor handle_cursors_[4];

  void updateHandles();
  Handle handleAt(const QPointF& pos);
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
  void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
  void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;

  void loadSize();
};

}

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_STICKYNOTEGRAPHICSITEM_H_

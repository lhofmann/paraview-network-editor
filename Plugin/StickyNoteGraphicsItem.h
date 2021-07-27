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
 protected:
  void paint(QPainter *p, const QStyleOptionGraphicsItem *options, QWidget *widget) override;
};

}

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_STICKYNOTEGRAPHICSITEM_H_

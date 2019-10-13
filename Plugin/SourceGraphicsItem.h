#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_SOURCEGRAPHICSITEM_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_SOURCEGRAPHICSITEM_H_

#include "EditorGraphicsItem.h"

class pqPipelineSource;

class SourceGraphicsItem : public EditorGraphicsItem {
 public:
  SourceGraphicsItem(pqPipelineSource* source);
  virtual ~SourceGraphicsItem();

  static const QSizeF size_;

 protected:
  void paint(QPainter* p, const QStyleOptionGraphicsItem* options, QWidget* widget) override;

 private:
  pqPipelineSource* source_;
};

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_SOURCEGRAPHICSITEM_H_

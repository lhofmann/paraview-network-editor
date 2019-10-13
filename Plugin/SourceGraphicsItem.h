#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_SOURCEGRAPHICSITEM_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_SOURCEGRAPHICSITEM_H_

#include "EditorGraphicsItem.h"
#include "LabelGraphicsItem.h"

class pqPipelineSource;
class pqServerManagerModelItem;

class SourceGraphicsItem : public QObject, public EditorGraphicsItem, public LabelGraphicsItemObserver {
  Q_OBJECT
 public:
  SourceGraphicsItem(pqPipelineSource* source);
  virtual ~SourceGraphicsItem();

  static const QSizeF size_;

  void editIdentifier();

  pqPipelineSource* getSource() const;

 protected:
  void paint(QPainter* p, const QStyleOptionGraphicsItem* options, QWidget* widget) override;

  // LabelGraphicsItem overrides
  void onLabelGraphicsItemChanged(LabelGraphicsItem* item) override;
  void onLabelGraphicsItemEdited(LabelGraphicsItem* item) override;

  // pqPipelineSource signals
  void onSourceNameChanged(pqServerManagerModelItem*);

 private:
  void positionLablels();

  pqPipelineSource* source_;
  LabelGraphicsItem* identifierLabel_;
  LabelGraphicsItem* typeLabel_;
};

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_SOURCEGRAPHICSITEM_H_

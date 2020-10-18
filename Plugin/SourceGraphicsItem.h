#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_SOURCEGRAPHICSITEM_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_SOURCEGRAPHICSITEM_H_

#include "EditorGraphicsItem.h"
#include "LabelGraphicsItem.h"

class pqPipelineFilter;
class pqPipelineSource;
class pqServerManagerModelItem;

namespace ParaViewNetworkEditor {

class InputPortGraphicsItem;
class OutputPortGraphicsItem;

class SourceGraphicsItem : public QObject, public EditorGraphicsItem, public LabelGraphicsItemObserver {
 Q_OBJECT
 public:
  SourceGraphicsItem(pqPipelineSource *source);
  virtual ~SourceGraphicsItem();

  static const QSizeF size_;

  void editIdentifier();

  pqPipelineSource *getSource() const;
  void aboutToRemoveSource();

  enum class PortType { In, Out };
  static QPointF portOffset(PortType type, size_t index);
  QPointF portPosition(PortType type, size_t index);

  InputPortGraphicsItem *getInputPortGraphicsItem(int) const;
  OutputPortGraphicsItem *getOutputPortGraphicsItem(int) const;

  void storePosition();
  void loadPosition();

  void showToolTip(QGraphicsSceneHelpEvent *e) override;

 protected:
  void paint(QPainter *p, const QStyleOptionGraphicsItem *options, QWidget *widget) override;

  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e) override;
  QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

  // LabelGraphicsItem overrides
  void onLabelGraphicsItemChanged(LabelGraphicsItem *item) override;
  void onLabelGraphicsItemEdited(LabelGraphicsItem *item) override;

  // pqPipelineSource signals
  void onSourceNameChanged(pqServerManagerModelItem *);

  void addInport(int);
  void addOutport(int);

 private:
  void positionLablels();

  pqPipelineSource *source_;
  LabelGraphicsItem *identifierLabel_;
  LabelGraphicsItem *typeLabel_;

  std::vector<InputPortGraphicsItem *> inportItems_;
  std::vector<OutputPortGraphicsItem *> outportItems_;

  bool positionModified_ = false;

};

}

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_SOURCEGRAPHICSITEM_H_

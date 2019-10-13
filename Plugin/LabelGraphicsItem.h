#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_LABELGRAPHICSITEM_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_LABELGRAPHICSITEM_H_

#include "observer.h"
#include <QGraphicsTextItem>

class LabelGraphicsItem;

class LabelGraphicsItemObserver : public Observer {
 public:
  LabelGraphicsItemObserver() = default;
  virtual ~LabelGraphicsItemObserver() = default;

  virtual void onLabelGraphicsItemChanged(LabelGraphicsItem *) {};

  virtual void onLabelGraphicsItemEdited(LabelGraphicsItem *) {};
};
class LabelGraphicsItemObservable
    : public Observable<LabelGraphicsItemObserver> {
 public:
  LabelGraphicsItemObservable() = default;
  virtual ~LabelGraphicsItemObservable() = default;
  void notifyObserversChanged(LabelGraphicsItem *);
  void notifyObserversEdited(LabelGraphicsItem *);
};

class LabelGraphicsItem : public QGraphicsTextItem,
                          public LabelGraphicsItemObservable {

 public:
  LabelGraphicsItem(QGraphicsItem *parent, int width = 30,
                    Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignTop);
  virtual ~LabelGraphicsItem() = default;

  QString text() const;
  void setText(const QString &);
  void setHtml(const QString &str);

  QString croppedText() const;
  void setCrop(int width);
  bool isCropped() const;

  int usedTextWidth() const;

  void setNoFocusOut();
  bool isFocusOut() const;

  void setAlignment(Qt::Alignment alignment);

 protected:
  virtual void keyPressEvent(QKeyEvent *keyEvent) override;
  virtual void focusInEvent(QFocusEvent *event) override;
  virtual void focusOutEvent(QFocusEvent *event) override;

  void updatePosition();

 private:
  int width_;
  bool focusOut_;
  QString orgText_;
  Qt::Alignment alignment_;  // Qt::AlignLeft/Right/HCenter | Qt::AlignTop/Bottom/VCenter
};

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_LABELGRAPHICSITEM_H_

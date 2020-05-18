#include "LabelGraphicsItem.h"

#include <QFocusEvent>
#include <QFont>
#include <QPainter>
#include <QTextCursor>
#include <QFontMetrics>
#include <QTextDocument>

namespace ParaViewNetworkEditor {

LabelGraphicsItem::LabelGraphicsItem(QGraphicsItem *parent, int width, Qt::Alignment alignment)
    : QGraphicsTextItem(parent),
      LabelGraphicsItemObservable(),
      width_(width),
      focusOut_(false),
      orgText_(""),
      alignment_(alignment) {
  font().setPixelSize(4);
  document()->setDocumentMargin(1.0);
}

QString LabelGraphicsItem::text() const {
  if (isCropped())
    return orgText_;
  else
    return toPlainText();
}

void LabelGraphicsItem::setText(const QString &str) {
  QFontMetrics fm = QFontMetrics(font());
  auto text = fm.elidedText(str, Qt::ElideMiddle, width_);
  setPlainText(text);
  setToolTip(str);
  orgText_ = str;

  updatePosition();
}

void LabelGraphicsItem::setHtml(const QString &str) {
  QGraphicsTextItem::setHtml(str);
  updatePosition();
}

void LabelGraphicsItem::updatePosition() {
  // adjust transformation
  QRectF b = QGraphicsTextItem::boundingRect();
  double x = b.x();
  double y = b.y();
  // horizontal alignment
  if (alignment_ & Qt::AlignHCenter) {
    x -= b.width() / 2.0;
  } else if (alignment_ & Qt::AlignRight) {
    x -= b.width();
  }
  // vertical alignment
  if (alignment_ & Qt::AlignVCenter) {
    y -= b.height() / 2.0;
  } else if (alignment_ & Qt::AlignBottom) {
    y -= b.height();
  }

  QTransform t(QTransform::fromTranslate(x, y));
  setTransform(t);
}

QString LabelGraphicsItem::croppedText() const { return toPlainText(); }

void LabelGraphicsItem::setCrop(int width) {
  width_ = width;
  setText(orgText_);
}

bool LabelGraphicsItem::isCropped() const { return (orgText_ != toPlainText()); }

int LabelGraphicsItem::usedTextWidth() const {
  QFontMetrics fm = QFontMetrics(font());
  return fm.tightBoundingRect(croppedText()).width();
}

void LabelGraphicsItem::setNoFocusOut() { focusOut_ = false; }

bool LabelGraphicsItem::isFocusOut() const { return focusOut_; }

void LabelGraphicsItem::setAlignment(Qt::Alignment alignment) {
  QGraphicsTextItem::prepareGeometryChange();
  alignment_ = alignment;
  updatePosition();
}

void LabelGraphicsItem::keyPressEvent(QKeyEvent *keyEvent) {
  if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
    clearFocus();
  } else {
    QGraphicsTextItem::keyPressEvent(keyEvent);
    keyEvent->accept();
  }
  notifyObserversEdited(this);
}

void LabelGraphicsItem::focusInEvent(QFocusEvent *) {
  if (isCropped()) setPlainText(orgText_);
}

void LabelGraphicsItem::focusOutEvent(QFocusEvent *) {
  focusOut_ = true;
  setFlags(nullptr);
  setTextInteractionFlags(Qt::NoTextInteraction);
  QTextCursor cur = QTextCursor(textCursor());
  cur.clearSelection();
  setTextCursor(cur);
  setText(toPlainText());
  notifyObserversChanged(this);
  focusOut_ = false;
}

void LabelGraphicsItemObservable::notifyObserversChanged(LabelGraphicsItem *item) {
  forEachObserver([&](LabelGraphicsItemObserver *o) { o->onLabelGraphicsItemChanged(item); });
}

void LabelGraphicsItemObservable::notifyObserversEdited(LabelGraphicsItem *item) {
  forEachObserver([&](LabelGraphicsItemObserver *o) { o->onLabelGraphicsItemEdited(item); });
}

}

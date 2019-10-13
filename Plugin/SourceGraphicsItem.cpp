#include "SourceGraphicsItem.h"
#include <pqPipelineSource.h>
#include <pqServerManagerModelItem.h>
#include <QPainter>
#include <QTextCursor>
#include <vtkSMSourceProxy.h>

const QSizeF SourceGraphicsItem::size_ = {150.f, 50.f};

int pointSizeToPixelSize(const int pointSize) {
  // compute pixel size for fonts by assuming 96 dpi as basis
  return ((pointSize * 4) / 3);
}

SourceGraphicsItem::SourceGraphicsItem(pqPipelineSource *source)
: source_(source)
{
  static constexpr int labelHeight = 8;
  auto width = static_cast<int>(size_.width());

  setZValue(SOURCEGRAPHICSITEM_DEPTH);
  setFlags(ItemIsMovable | ItemIsSelectable | ItemIsFocusable | ItemSendsGeometryChanges);
  setRect(-size_.width() / 2, -size_.height() / 2, size_.width(), size_.height());

  {
    identifierLabel_ =
        new LabelGraphicsItem(this, width - 2 * labelHeight - 10, Qt::AlignBottom);
    identifierLabel_->setDefaultTextColor(Qt::white);
    QFont nameFont("Segoe", labelHeight, QFont::Black, false);
    nameFont.setPixelSize(pointSizeToPixelSize(labelHeight));
    identifierLabel_->setFont(nameFont);
    identifierLabel_->setText(source->getSMName());
    LabelGraphicsItemObserver::addObservation(identifierLabel_);
  }
  {
    typeLabel_ = new LabelGraphicsItem(this, width - 2 * labelHeight, Qt::AlignTop);
    typeLabel_->setDefaultTextColor(Qt::lightGray);
    QFont classFont("Noto Sans", labelHeight, QFont::Normal, false);
    classFont.setPixelSize(pointSizeToPixelSize(labelHeight));
    typeLabel_->setFont(classFont);

    auto smproxy = source->getSourceProxy();
    // std::string label = std::string(smproxy->GetVTKClassName()) + " (" + std::string(smproxy->GetXMLLabel()) + ")";
    typeLabel_->setText(smproxy->GetVTKClassName());
    LabelGraphicsItemObserver::addObservation(typeLabel_);
  }
  positionLablels();

  connect(source_, &pqPipelineSource::nameChanged, this, &SourceGraphicsItem::onSourceNameChanged);
}

SourceGraphicsItem::~SourceGraphicsItem() = default;

void SourceGraphicsItem::paint(QPainter *p, const QStyleOptionGraphicsItem *options, QWidget *widget) {
  const float roundedCorners = 9.0f;

  p->save();
  p->setRenderHint(QPainter::Antialiasing, true);
  QColor selectionColor("#7a191b");
  QColor backgroundColor("#3b3d3d");
  QColor borderColor("#282828");

  if (isSelected()) {
    p->setBrush(selectionColor);
  } else {
    p->setBrush(backgroundColor);
  }
  p->setPen(QPen(QBrush(borderColor), 2.0));

  p->drawRoundedRect(rect(), roundedCorners, roundedCorners);

  p->restore();
}

void SourceGraphicsItem::editIdentifier() {
  setFocus();
  identifierLabel_->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);

  QTextCursor cur = identifierLabel_->textCursor();
  cur.movePosition(QTextCursor::End);
  identifierLabel_->setTextCursor(cur);
  identifierLabel_->setTextInteractionFlags(Qt::TextEditorInteraction);
  identifierLabel_->setFocus();
  identifierLabel_->setSelected(true);
}

void SourceGraphicsItem::positionLablels() {
  static constexpr int labelMargin = 7;

  identifierLabel_->setPos(QPointF(rect().left() + labelMargin, -2));
  typeLabel_->setPos(QPointF(rect().left() + labelMargin, -3));
}

void SourceGraphicsItem::onLabelGraphicsItemChanged(LabelGraphicsItem* item) {
  if (item == identifierLabel_ && identifierLabel_->isFocusOut()) {
    QString newName = identifierLabel_->text();
    if (newName != "" && newName != source_->getSMName()) {
      source_->rename(newName);
      identifierLabel_->setNoFocusOut();
    }
  }
}

void SourceGraphicsItem::onLabelGraphicsItemEdited(LabelGraphicsItem*) {
  positionLablels();
}

void SourceGraphicsItem::onSourceNameChanged(pqServerManagerModelItem*) {
  QString newName = source_->getSMName();
  if (newName != identifierLabel_->text()) {
    identifierLabel_->setText(newName);
  }
}
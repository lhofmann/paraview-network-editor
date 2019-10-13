#include "SourceGraphicsItem.h"
#include "PortGraphicsItem.h"

#include <pqPipelineSource.h>
#include <pqPipelineFilter.h>
#include <pqOutputPort.h>
#include <pqServerManagerModelItem.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSMInputProperty.h>
#include <vtkSMDomain.h>
#include <vtkSMDomainIterator.h>
#include <vtkSMDataTypeDomain.h>
#include <vtkPVDataInformation.h>

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

  for (int i = 0; i < source->getNumberOfOutputPorts(); ++i) {
    this->addOutport(source, i);
    auto op = source->getOutputPort(i);
    std::cout << "Output " << i << ": " << op->getPortName().toStdString() << " | " << op->getDataInformation()->GetDataSetTypeAsString() << std::endl;
  }

  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(source);
  if (filter) {
    std::vector<vtkSMInputProperty*> inputs(filter->getNumberOfInputPorts());
    vtkSMPropertyIterator* propIter = filter->getSourceProxy()->NewPropertyIterator();
    for (propIter->Begin(); !propIter->IsAtEnd(); propIter->Next()) {
      if (auto prop = vtkSMInputProperty::SafeDownCast(propIter->GetProperty())) {
        if (prop->GetPortIndex() >= 0 && prop->GetPortIndex() < inputs.size()) {
          inputs[prop->GetPortIndex()] = prop;
        }
      }
    }

    for (int i = 0; i < filter->getNumberOfInputPorts(); ++i) {
      this->addInport(filter, i);
      if (!inputs[i]) {
        std::cout << "Error on input " << i << std::endl;
        continue;
      }

      QStringList types;
      auto domIter = inputs[i]->NewDomainIterator();
      for (domIter->Begin(); !domIter->IsAtEnd(); domIter->Next()) {
        auto domain = domIter->GetDomain();
        if (domain->IsA("vtkSMDataTypeDomain")) {
          auto dtd = static_cast<vtkSMDataTypeDomain*>(domain);
          for (unsigned int cc = 0; cc < dtd->GetNumberOfDataTypes(); cc++) {
            types << dtd->GetDataType(cc);
          }
        }
      }
      std::cout << "Input " << i << ": " << filter->getInputPortName(i).toStdString() << " | " << types.join(" or ").toStdString() << std::endl;
    }
  }

  connect(source_, &pqPipelineSource::nameChanged, this, &SourceGraphicsItem::onSourceNameChanged);
}

SourceGraphicsItem::~SourceGraphicsItem() = default;

pqPipelineSource* SourceGraphicsItem::getSource() const {
  return source_;
}

QPointF SourceGraphicsItem::portOffset(PortType type, size_t index) {
  const QPointF offset = {12.5f, (type == PortType::In ? 1.0f : -1.0f) * 4.5f};
  const QPointF delta = {12.5f, 0.0f};
  const QPointF rowDelta = {0.0f, (type == PortType::In ? -1.0f : 1.0f) * 12.5f};
  const size_t portsPerRow = 10;

  auto poffset = QPointF{-SourceGraphicsItem::size_.width() / 2,
                         (type == PortType::In ? -SourceGraphicsItem::size_.height() / 2
                                               : SourceGraphicsItem::size_.height() / 2)};

  return poffset + offset + rowDelta * static_cast<qreal>(index / portsPerRow) +
      delta * static_cast<qreal>(index % portsPerRow);
}

QPointF SourceGraphicsItem::portPosition(PortType type, size_t index) {
  return rect().center() + portOffset(type, index);
}

void SourceGraphicsItem::addInport(pqPipelineFilter* filter, int port) {
  auto pos = portPosition(PortType::In, inportItems_.size());
  inportItems_.emplace_back(new InputPortGraphicsItem(this, pos, filter, port));
}

void SourceGraphicsItem::addOutport(pqPipelineSource* source, int port) {
  auto pos = portPosition(PortType::Out, outportItems_.size());
  outportItems_.emplace_back(new OutputPortGraphicsItem(this, pos, source, port));
}

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

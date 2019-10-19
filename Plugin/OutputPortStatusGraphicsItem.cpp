#include "OutputPortStatusGraphicsItem.h"

#include <vtkSMParaViewPipelineControllerWithRendering.h>

#include <pqOutputPort.h>
#include <pqActiveObjects.h>
#include <pqView.h>
#include <vtkSMViewProxy.h>

#include <QPen>
#include <QPainter>
#include <QBrush>
#include <QColor>

OutputPortStatusGraphicsItem::OutputPortStatusGraphicsItem(QGraphicsRectItem* parent, pqOutputPort* port)
    : EditorGraphicsItem(parent)
    , size_(9.0f)
    , lineWidth_(1.0f)
    , port_(port)
{
  setRect(-0.5f * size_ - lineWidth_, -0.5f * size_ - lineWidth_, size_ + 2.0 * lineWidth_,
          size_ + 2.0 * lineWidth_);
}

void OutputPortStatusGraphicsItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) {
  qreal ledRadius = size_ / 2.0f;
  QColor baseColor = QColor(0, 170, 0).lighter(200);

  QColor ledColor;
  QColor borderColor(124, 124, 124);

  auto controller = vtkSmartPointer<vtkSMParaViewPipelineControllerWithRendering>::New();
  pqView* activeView = pqActiveObjects::instance().activeView();
  vtkSMViewProxy* viewProxy = activeView ? activeView->getViewProxy() : nullptr;
  bool visible = (viewProxy == nullptr
      ? false
      : (controller->GetVisibility(port_->getSourceProxy(), port_->getPortNumber(), viewProxy)));

  if (visible) {
    ledColor = baseColor;
  } else {
    ledColor = baseColor.darker(400);
  }

  p->save();
  p->setPen(QPen(borderColor, 1.0));
  p->setRenderHint(QPainter::Antialiasing, true);
  p->setBrush(QBrush(ledColor));
  p->drawEllipse(QPointF(0.0f, 0.0f), ledRadius, ledRadius);
  p->restore();
}

void OutputPortStatusGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent* e) {
  pqActiveObjects::instance().setActivePort(port_);
}

void OutputPortStatusGraphicsItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e) {
  auto controller = vtkSmartPointer<vtkSMParaViewPipelineControllerWithRendering>::New();
  pqView* activeView = pqActiveObjects::instance().activeView();
  vtkSMViewProxy* viewProxy = activeView ? activeView->getViewProxy() : nullptr;
  if (!viewProxy)
    return;
  bool visible = controller->GetVisibility(port_->getSourceProxy(), port_->getPortNumber(), viewProxy);
  controller->SetVisibility(port_->getSourceProxy(), port_->getPortNumber(), viewProxy, !visible);
  activeView->render();
}
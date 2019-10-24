#include "OutputPortStatusGraphicsItem.h"

#include <vtkSMParaViewPipelineControllerWithRendering.h>
#include <vtkSMPVRepresentationProxy.h>

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
  setRect(-0.5f * size_ - lineWidth_, -0.5f * size_ - lineWidth_, 1.5 * size_ + 2.0 * lineWidth_,
          size_ + 2.0 * lineWidth_);
}

void OutputPortStatusGraphicsItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) {
  qreal ledRadius = size_ / 2.0f;
  QColor baseColor = QColor(0, 170, 0).lighter(200);

  QColor ledColor;
  QColor borderColor(124, 124, 124);

  bool visible = false;
  bool scalar_bar = false;

  auto controller = vtkSmartPointer<vtkSMParaViewPipelineControllerWithRendering>::New();
  pqView* activeView = pqActiveObjects::instance().activeView();
  vtkSMViewProxy* viewProxy = activeView ? activeView->getViewProxy() : nullptr;
  if (viewProxy) {
    visible = controller->GetVisibility(port_->getSourceProxy(), port_->getPortNumber(), viewProxy);
    pqDataRepresentation* representation = port_->getRepresentation(activeView);
    if (representation) {
      scalar_bar = vtkSMPVRepresentationProxy::IsScalarBarVisible(representation->getProxy(), viewProxy);
    }
  }

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

  if (scalar_bar) {
    p->setPen(QPen(borderColor, 1.0));
    qreal radius = ledRadius; // + lineWidth_;
    QRectF rect(radius, -radius, radius, 2. * radius);

    QLinearGradient lgrad(rect.topLeft(), rect.bottomLeft());
    lgrad.setColorAt(0., "#4747db");
    lgrad.setColorAt(1. / 7., "#00005c");
    lgrad.setColorAt(2. / 7., "#00ffff");
    lgrad.setColorAt(3. / 7., "#008000");
    lgrad.setColorAt(4. / 7., "#ffff00");
    lgrad.setColorAt(5. / 7., "#ff6100");
    lgrad.setColorAt(6. / 7., "#6b0000");
    lgrad.setColorAt(1., "#e04d4d");
    /*
    lgrad.setColorAt(0., "#0c366a");
    lgrad.setColorAt(1. / 16., "#114170");
    lgrad.setColorAt(2. / 16., "#184c76");
    lgrad.setColorAt(3. / 16., "#21577b");
    lgrad.setColorAt(4. / 16., "#306180");
    lgrad.setColorAt(5. / 16., "#446b83");
    lgrad.setColorAt(6. / 16., "#5a7484");
    lgrad.setColorAt(7. / 16., "#707d85");
    lgrad.setColorAt(8. / 16., "#858585");
    lgrad.setColorAt(9. / 16., "#998e88");
    lgrad.setColorAt(10. / 16., "#b3988a");
    lgrad.setColorAt(11. / 16., "#c29e8b");
    lgrad.setColorAt(12. / 16., "#d7a68c");
    lgrad.setColorAt(13. / 16., "#e9af8e");
    lgrad.setColorAt(14. / 16., "#f3bc93");
    lgrad.setColorAt(15. / 16., "#f9ca9a");
    lgrad.setColorAt(1., "#fdd7a2");
     */
    p->setBrush(lgrad);
    p->drawRect(rect);

    qreal linewidth = 0.75;
    p->setPen(QPen(borderColor, linewidth));
    p->drawLine(rect.topLeft() + QPointF(0., linewidth / 2.), rect.bottomLeft() - QPointF(0., linewidth / 2.));
  }

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
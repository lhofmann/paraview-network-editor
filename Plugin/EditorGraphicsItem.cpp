#include "EditorGraphicsItem.h"
#include "NetworkEditor.h"
#include "utilpq.h"

#include <pqApplicationCore.h>
#include <pqServerManagerModel.h>
#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqOutputPort.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMInputProperty.h>
#include <vtkSMPropertyHelper.h>

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QToolTip>

namespace ParaViewNetworkEditor {

EditorGraphicsItem::EditorGraphicsItem() : QGraphicsRectItem() {}

EditorGraphicsItem::EditorGraphicsItem(QGraphicsItem *parent) : QGraphicsRectItem(parent) {}

EditorGraphicsItem::~EditorGraphicsItem() = default;

QPoint EditorGraphicsItem::mapPosToSceen(QPointF inPos) const {
  if (scene() != nullptr                                  // the focus item belongs to a scene
      && !scene()->views().isEmpty()                      // that scene is displayed in a view...
      && scene()->views().first() != nullptr              // ... which is not null...
      && scene()->views().first()->viewport() != nullptr  // ... and has a viewport
      ) {
    QPointF sceneP = mapToScene(inPos);
    QGraphicsView *v = scene()->views().first();
    QPoint viewP = v->mapFromScene(sceneP);
    return v->viewport()->mapToGlobal(viewP);
  } else {
    return QPoint(0, 0);
  }
}

void EditorGraphicsItem::showToolTip(QGraphicsSceneHelpEvent *) {}

void EditorGraphicsItem::showToolTipHelper(QGraphicsSceneHelpEvent *e, QString string) const {
  QGraphicsView *v = scene()->views().first();
  QRectF rect = this->mapRectToScene(this->rect());
  QRect viewRect = v->mapFromScene(rect).boundingRect();
  e->accept();
  QToolTip::showText(e->screenPos(), string, v, viewRect);
}

NetworkEditor *EditorGraphicsItem::getNetworkEditor() const {
  return qobject_cast<NetworkEditor *>(scene());
}

void EditorGraphicsItem::showSourceInfo(QGraphicsSceneHelpEvent *event, pqPipelineSource* source) const {
  if (!source)
    return;
  QString s;
  s += "<html><body><table style=\"white-space: nowrap;\">";
  s += "<tr><td>" + source->getSMName() + " (" + QString(source->getSourceProxy()->GetVTKClassName()) + ") </td></tr>";
  s += "</table></body></html>";
  this->showToolTipHelper(event, s);
}

void EditorGraphicsItem::showInportInfo(QGraphicsSceneHelpEvent *event, pqPipelineFilter* filter, int port) const {
  if (!filter)
    return;
  QString s;
  s += "<html><body><table style=\"white-space: nowrap;\">";
  QString optional = utilpq::optional_input(filter, port) ? "(optional)" : "";
  s += "<tr><td>" + filter->getInputPortName(port) + optional + "</td></tr>";

  const char *input_name = filter->getInputPortName(port).toLocal8Bit().constData();
  auto prop = vtkSMInputProperty::SafeDownCast(filter->getProxy()->GetProperty(input_name));
  auto smModel = pqApplicationCore::instance()->getServerManagerModel();
  vtkSMPropertyHelper helper(prop);
  unsigned int num_proxies = helper.GetNumberOfElements();
  for (unsigned int i = 0; i < num_proxies; ++i) {
    auto proxy_source = smModel->findItem<pqPipelineSource *>(helper.GetAsProxy(i));
    int output_id = helper.GetOutputPort(i);
    QString name = proxy_source->getSMName();
    QString source_type = proxy_source->getSourceProxy()->GetVTKClassName();
    QString port_name = proxy_source->getOutputPort(output_id)->getPortName();
    s += "<tr><td>" + name +" (" + source_type + ") " + port_name +" (" + QString::number(output_id) + ")</td></tr>";
  }
  s += "</table></body></html>";
  this->showToolTipHelper(event, s);
}

void EditorGraphicsItem::showOutportInfo(QGraphicsSceneHelpEvent *event, pqPipelineSource* source, int port) const {
  if (!source)
    return;
  QString s;
  s += "<html><body><table style=\"white-space: nowrap;\">";
  s += "<tr><td>" + source->getOutputPort(port)->getPortName() + " (" + QString::number(port) + ")</td></tr>";
  pqOutputPort* output_port = source->getOutputPort(port);
  const int n = source->getNumberOfConsumers(port);
  for (int i = 0; i < n; ++i) {
    pqPipelineFilter* consumer = static_cast<pqPipelineFilter*>(source->getConsumer(port, i));

    QString name = consumer->getSMName();
    QString source_type = consumer->getSourceProxy()->GetVTKClassName();

    auto named_inputs = consumer->getNamedInputs();
    for (auto iter = named_inputs.constBegin(); iter != named_inputs.constEnd(); ++iter) {
      if (iter.value().contains(output_port)) {
        s += "<tr><td>" + name +" (" + source_type + ") " + iter.key() + "</td></tr>";
      }
    }
  }
  s += "</table></body></html>";
  this->showToolTipHelper(event, s);
}

void EditorGraphicsItem::showConnectionInfo(QGraphicsSceneHelpEvent *event,
                                            pqPipelineSource* source, int output_port,
                                            pqPipelineFilter* dest, int input_port) const {

  QString s;
  s += "<html><body><table style=\"white-space: nowrap;\">";
  s += "<tr><td>" + source->getSMName() +" (" + QString(source->getSourceProxy()->GetVTKClassName()) + ") "
      + source->getOutputPort(output_port)->getPortName() + " (" + QString::number(output_port) + ")</td></tr>";
  s += "<tr><td>" + dest->getSMName() +" (" + QString(dest->getSourceProxy()->GetVTKClassName()) + ") "
      + dest->getInputPortName(input_port) + " (" + QString::number(input_port) + ")</td></tr>";
  s += "</table></body></html>";
  this->showToolTipHelper(event, s);
}

}

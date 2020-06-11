#include "utilpq.h"

#include <vtkSMProxy.h>
#include <vtkSMProperty.h>
#include <vtkSMInputProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkPVXMLElement.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyIterator.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMParaViewPipelineControllerWithRendering.h>
#include <vtkSMPVRepresentationProxy.h>
#include <vtkSMViewProxy.h>
#include <vtkSMDataTypeDomain.h>
#include <vtkPVDataInformation.h>
#include <vtkDataObject.h>
#include <vtkDataSet.h>
#include <vtkSmartPointer.h>
#include <vtkDataObjectTypes.h>

#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqOutputPort.h>
#include <pqApplicationCore.h>
#include <pqServerManagerModel.h>
#include <pqActiveObjects.h>

namespace ParaViewNetworkEditor {
namespace utilpq {

static vtkSMParaViewPipelineControllerWithRendering *controller = vtkSMParaViewPipelineControllerWithRendering::New();

bool multiple_inputs(pqPipelineFilter *filter, int port) {
  QString input_name = filter->getInputPortName(port);
  vtkSMInputProperty *ip = vtkSMInputProperty::SafeDownCast(
      filter->getProxy()->GetProperty(input_name.toLocal8Bit().data()));
  return ip->GetMultipleInput();
}

bool optional_input(pqPipelineFilter *filter, int port) {
  QString input_name = filter->getInputPortName(port);
  vtkSMInputProperty *ip = vtkSMInputProperty::SafeDownCast(
      filter->getProxy()->GetProperty(input_name.toLocal8Bit().data()));
  vtkPVXMLElement *hints = ip->GetHints();
  return hints && hints->FindNestedElementByName("Optional");
}

bool filter_reachable(pqPipelineFilter *dest, pqPipelineSource *source) {
  if (qobject_cast<pqPipelineSource *>(dest) == source)
    return true;
  QList<pqPipelineSource *> consumers = source->getAllConsumers();
  for (pqPipelineSource *consumer : consumers) {
    if (filter_reachable(dest, consumer)) {
      return true;
    }
  }
  return false;
}

bool can_connect(pqPipelineSource *source, int out_port, pqPipelineFilter *dest, int in_port) {
  QString input_name = dest->getInputPortName(in_port);
  vtkSMInputProperty *input = vtkSMInputProperty::SafeDownCast(
      dest->getProxy()->GetProperty(input_name.toLocal8Bit().data()));

  input->RemoveAllUncheckedProxies();
  input->AddUncheckedInputConnection(source->getProxy(), out_port);
  bool result = input->IsInDomains() > 0;
  input->RemoveAllUncheckedProxies();

  if (auto filter = qobject_cast<pqPipelineFilter *>(source)) {
    result = result && !filter_reachable(filter, qobject_cast<pqPipelineSource *>(dest));
  }

  return result;
}

void add_connection(pqPipelineSource *source, int out_port, pqPipelineFilter *dest, int in_port) {
  QString input_name = dest->getInputPortName(in_port);
  std::vector<vtkSMProxy *> inputPtrs;
  std::vector<unsigned int> inputPorts;
  vtkSMPropertyHelper helper(dest->getProxy(), input_name.toLocal8Bit().data());
  unsigned int numProxies = helper.GetNumberOfElements();
  for (unsigned int cc = 0; cc < numProxies; cc++) {
    inputPtrs.push_back(helper.GetAsProxy(cc));
    inputPorts.push_back(helper.GetOutputPort(cc));
  }
  inputPtrs.push_back(source->getProxy());
  inputPorts.push_back(out_port);

  vtkSMInputProperty *ip = vtkSMInputProperty::SafeDownCast(
      dest->getProxy()->GetProperty(input_name.toLocal8Bit().data()));
  ip->SetProxies(static_cast<unsigned int>(inputPtrs.size()), &inputPtrs[0], &inputPorts[0]);

  dest->getProxy()->UpdateVTKObjects();
  pqApplicationCore::instance()->render();
}

void remove_connection(pqPipelineSource *source, int out_port, pqPipelineFilter *dest, int in_port) {
  QString input_name = dest->getInputPortName(in_port);
  std::vector<vtkSMProxy *> inputPtrs;
  std::vector<unsigned int> inputPorts;
  vtkSMPropertyHelper helper(dest->getProxy(), input_name.toLocal8Bit().data());
  unsigned int numProxies = helper.GetNumberOfElements();
  for (unsigned int cc = 0; cc < numProxies; cc++) {
    vtkSMProxy *proxy = helper.GetAsProxy(cc);
    unsigned int port = helper.GetOutputPort(cc);
    if (proxy == source->getProxy() && (int) port == out_port) {
      continue;
    }
    inputPtrs.push_back(helper.GetAsProxy(cc));
    inputPorts.push_back(helper.GetOutputPort(cc));
  }

  vtkSMInputProperty *ip = vtkSMInputProperty::SafeDownCast(
      dest->getProxy()->GetProperty(input_name.toLocal8Bit().data()));
  ip->SetProxies(static_cast<unsigned int>(inputPtrs.size()), &inputPtrs[0], &inputPorts[0]);

  dest->getProxy()->UpdateVTKObjects();
  pqApplicationCore::instance()->render();
}

void clear_connections(pqPipelineFilter *filter, int port) {
  QString input_name = filter->getInputPortName(port);
  vtkSMInputProperty *ip = vtkSMInputProperty::SafeDownCast(
      filter->getProxy()->GetProperty(input_name.toLocal8Bit().data()));
  ip->RemoveAllProxies();
}

std::vector<pqPipelineSource *> get_sources() {
  auto session = vtkSMProxyManager::GetProxyManager()->GetActiveSession();
  if (!session)
    return {};
  auto iter = vtkSmartPointer<vtkSMProxyIterator>::New();
  iter->SetSession(session);
  iter->SetModeToOneGroup();
  std::vector<pqPipelineSource *> result;
  auto smModel = pqApplicationCore::instance()->getServerManagerModel();
  for (iter->Begin("sources"); !iter->IsAtEnd(); iter->Next()) {
    auto proxy = iter->GetProxy();
    auto source_proxy = vtkSMSourceProxy::SafeDownCast(proxy);
    if (!source_proxy)
      continue;
    auto pipeline_source = smModel->findItem<pqPipelineSource *>(source_proxy);
    if (pipeline_source)
      result.emplace_back(pipeline_source);
  }
  std::sort(result.begin(), result.end(), [](pqPipelineSource *a, pqPipelineSource *b) -> bool {
    return a->getProxy()->GetGlobalID() < b->getProxy()->GetGlobalID();
  });
  return result;
}

std::vector<pqView*> get_views() {
  auto session = vtkSMProxyManager::GetProxyManager()->GetActiveSession();
  if (!session)
    return {};
  auto iter = vtkSmartPointer<vtkSMProxyIterator>::New();
  iter->SetSession(session);
  iter->SetModeToOneGroup();
  std::vector<pqView *> result;
  auto smModel = pqApplicationCore::instance()->getServerManagerModel();
  for (iter->Begin("views"); !iter->IsAtEnd(); iter->Next()) {
    auto proxy = vtkSMViewProxy::SafeDownCast(iter->GetProxy());
    if (!proxy)
      continue;

    auto view = smModel->findItem<pqView *>(proxy);
    if (view)
      result.emplace_back(view);
  }
  return result;
}

std::pair<bool, bool> output_visibiility(pqPipelineSource *source, int out_port) {
  bool visible = false;
  bool scalar_bar = false;

  pqOutputPort *output = source->getOutputPort(out_port);

  pqView *activeView = pqActiveObjects::instance().activeView();
  vtkSMViewProxy *viewProxy = activeView ? activeView->getViewProxy() : nullptr;
  if (viewProxy) {
    visible = controller->GetVisibility(output->getSourceProxy(), out_port, viewProxy);
    pqDataRepresentation *representation = output->getRepresentation(activeView);
    if (representation) {
      scalar_bar = vtkSMPVRepresentationProxy::IsScalarBarVisible(representation->getProxy(), viewProxy);
    }
  }

  return {visible, scalar_bar};
}

void toggle_output_visibility(pqPipelineSource *source, int out_port) {
  pqView *activeView = pqActiveObjects::instance().activeView();
  vtkSMViewProxy *viewProxy = activeView ? activeView->getViewProxy() : nullptr;
  if (!viewProxy)
    return;
  bool visible = controller->GetVisibility(source->getSourceProxy(), out_port, viewProxy);
  controller->SetVisibility(source->getSourceProxy(), out_port, viewProxy, !visible);
}

void toggle_source_visibility(pqPipelineSource *source) {
  pqView *activeView = pqActiveObjects::instance().activeView();
  vtkSMViewProxy *viewProxy = activeView ? activeView->getViewProxy() : nullptr;
  if (!viewProxy)
    return;
  auto source_proxy = source->getSourceProxy();
  bool visible = false;
  for (int i = 0; i < source->getNumberOfOutputPorts(); ++i) {
    visible = visible || controller->GetVisibility(source_proxy, i, viewProxy);
  }
  for (int i = 0; i < source->getNumberOfOutputPorts(); ++i) {
    controller->SetVisibility(source_proxy, i, viewProxy, !visible);
  }
}

void set_source_visiblity(pqPipelineSource *source, bool visible) {
  pqView *activeView = pqActiveObjects::instance().activeView();
  vtkSMViewProxy *viewProxy = activeView ? activeView->getViewProxy() : nullptr;
  if (!viewProxy)
    return;
  auto source_proxy = source->getSourceProxy();
  for (int i = 0; i < source->getNumberOfOutputPorts(); ++i) {
    controller->SetVisibility(source_proxy, i, viewProxy, visible);
  }
}

void set_source_scalar_bar_visiblity(pqPipelineSource *source, bool visible) {
  pqView *activeView = pqActiveObjects::instance().activeView();
  vtkSMViewProxy *viewProxy = activeView ? activeView->getViewProxy() : nullptr;
  if (!viewProxy)
    return;
  for (int i = 0; i < source->getNumberOfOutputPorts(); ++i) {
    pqDataRepresentation *repr = source->getOutputPort(i)->getRepresentation(activeView);
    if (repr) {
      vtkSMPVRepresentationProxy::SetScalarBarVisibility(repr->getProxy(), viewProxy, visible);
    }
  }
}

std::vector<std::string> input_datatypes(pqPipelineFilter *filter, int in_port) {
  QString input_name = filter->getInputPortName(in_port);
  vtkSMInputProperty *ip = vtkSMInputProperty::SafeDownCast(
      filter->getProxy()->GetProperty(input_name.toLocal8Bit().data()));

  std::vector<std::string> result;
  auto domIter = ip->NewDomainIterator();
  for (domIter->Begin(); !domIter->IsAtEnd(); domIter->Next()) {
    auto domain = domIter->GetDomain();
    if (domain->IsA("vtkSMDataTypeDomain")) {
      auto dtd = static_cast<vtkSMDataTypeDomain *>(domain);
      for (unsigned int cc = 0; cc < dtd->GetNumberOfDataTypes(); cc++) {
        result.push_back(dtd->GetDataType(cc));
      }
    }
  }
  return result;
}

QColor output_dataset_color(pqPipelineSource *filter, int port_index) {
  if (!filter)
    return default_color;
  auto port = filter->getOutputPort(port_index);
  if (!port)
    return default_color;
  auto info = port->getDataInformation();
  if (!info)
    return default_color;

  if (info->GetCompositeDataSetType() > 0)
    return QColor(128, 64, 196);

  auto type = info->GetDataClassName();
  if (!type || vtkDataSet::IsTypeOf(type))
    return default_color;

  auto prototype = vtkSmartPointer<vtkDataObject>::Take(vtkDataObjectTypes::NewDataObject(info->GetDataSetType()));
  if (!prototype)
    return default_color;

  if (prototype->IsA("vtkImageData") || prototype->IsA("vtkRectilinearGrid") || prototype->IsA("vtkStructuredGrid"))
    return QColor(44, 123, 182);
  if (prototype->IsA("vtkUnstructuredGridBase"))
    return QColor(188, 101, 101);
  if (prototype->IsA("vtkPointSet"))
    return QColor(188, 188, 101);

  return default_color;
}

}
}

#include "utilpq.h"

#include <vtkSMProxy.h>
#include <vtkSMProperty.h>
#include <vtkSMInputProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkPVXMLElement.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyIterator.h>
#include <vtkSMSourceProxy.h>

#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqOutputPort.h>
#include <pqApplicationCore.h>
#include <pqServerManagerModel.h>

namespace utilpq {

bool multiple_inputs(pqPipelineFilter* filter, int port) {
  QString input_name = filter->getInputPortName(port);
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
      filter->getProxy()->GetProperty(input_name.toLocal8Bit().data()));
  return ip->GetMultipleInput();
}

bool optional_input(pqPipelineFilter* filter, int port) {
  QString input_name = filter->getInputPortName(port);
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
      filter->getProxy()->GetProperty(input_name.toLocal8Bit().data()));
  vtkPVXMLElement* hints = ip->GetHints();
  return hints && hints->FindNestedElementByName("Optional");
}

bool filter_reachable(pqPipelineFilter* dest, pqPipelineSource* source) {
  if (qobject_cast<pqPipelineSource*>(dest) == source)
    return true;
  QList<pqPipelineSource*> consumers = source->getAllConsumers();
  for (pqPipelineSource* consumer : consumers) {
    if (filter_reachable(dest, consumer)) {
      return true;
    }
  }
  return false;
}

bool can_connect(pqPipelineSource* source, int out_port, pqPipelineFilter* dest, int in_port) {
  QString input_name = dest->getInputPortName(in_port);
  vtkSMInputProperty* input = vtkSMInputProperty::SafeDownCast(
      dest->getProxy()->GetProperty(input_name.toLocal8Bit().data()));

  input->RemoveAllUncheckedProxies();
  input->AddUncheckedInputConnection(source->getProxy(), out_port);
  bool result = input->IsInDomains() > 0;
  input->RemoveAllUncheckedProxies();

  if (auto filter = qobject_cast<pqPipelineFilter*>(source)) {
    result = result && !filter_reachable(filter, qobject_cast<pqPipelineSource*>(dest));
  }

  return result;
}

void add_connection(pqPipelineSource* source, int out_port, pqPipelineFilter* dest, int in_port) {
  QString input_name = dest->getInputPortName(in_port);
  std::vector<vtkSMProxy*> inputPtrs;
  std::vector<unsigned int> inputPorts;
  vtkSMPropertyHelper helper(dest->getProxy(), input_name.toLocal8Bit().data());
  unsigned int numProxies = helper.GetNumberOfElements();
  for (unsigned int cc = 0; cc < numProxies; cc++) {
    inputPtrs.push_back(helper.GetAsProxy(cc));
    inputPorts.push_back(helper.GetOutputPort(cc));
  }
  inputPtrs.push_back(source->getProxy());
  inputPorts.push_back(out_port);

  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
      dest->getProxy()->GetProperty(input_name.toLocal8Bit().data()));
  ip->SetProxies(static_cast<unsigned int>(inputPtrs.size()), &inputPtrs[0], &inputPorts[0]);

  dest->getProxy()->UpdateVTKObjects();
  pqApplicationCore::instance()->render();
}

void remove_connection(pqPipelineSource* source, int out_port, pqPipelineFilter* dest, int in_port) {
  QString input_name = dest->getInputPortName(in_port);
  std::vector<vtkSMProxy*> inputPtrs;
  std::vector<unsigned int> inputPorts;
  vtkSMPropertyHelper helper(dest->getProxy(), input_name.toLocal8Bit().data());
  unsigned int numProxies = helper.GetNumberOfElements();
  for (unsigned int cc = 0; cc < numProxies; cc++) {
    vtkSMProxy* proxy = helper.GetAsProxy();
    unsigned int port = helper.GetOutputPort();
    if (proxy == source->getProxy() && port == out_port) {
      continue;
    }
    inputPtrs.push_back(helper.GetAsProxy(cc));
    inputPorts.push_back(helper.GetOutputPort(cc));
  }

  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
      dest->getProxy()->GetProperty(input_name.toLocal8Bit().data()));
  ip->SetProxies(static_cast<unsigned int>(inputPtrs.size()), &inputPtrs[0], &inputPorts[0]);

  dest->getProxy()->UpdateVTKObjects();
  pqApplicationCore::instance()->render();
}

void clear_connections(pqPipelineFilter* filter, int port) {
  QString input_name = filter->getInputPortName(port);
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
      filter->getProxy()->GetProperty(input_name.toLocal8Bit().data()));
  ip->RemoveAllProxies();
}

std::vector<pqPipelineSource*> get_sources() {
  auto session = vtkSMProxyManager::GetProxyManager()->GetActiveSession();
  if (!session)
    return {};
  auto iter = vtkSmartPointer<vtkSMProxyIterator>::New();
  iter->SetSession(session);
  iter->SetModeToOneGroup();
  std::vector<pqPipelineSource*> result;
  auto smModel = pqApplicationCore::instance()->getServerManagerModel();
  for (iter->Begin("sources"); !iter->IsAtEnd(); iter->Next()) {
    auto proxy = iter->GetProxy();
    auto source_proxy = vtkSMSourceProxy::SafeDownCast(proxy);
    if (!source_proxy)
      continue;
    auto pipeline_source = smModel->findItem<pqPipelineSource*>(source_proxy);
    if (pipeline_source)
      result.emplace_back(pipeline_source);
  }
  std::sort(result.begin(), result.end(), [](pqPipelineSource* a, pqPipelineSource* b) -> bool {
    return a->getProxy()->GetGlobalID() < b->getProxy()->GetGlobalID();
  });
  return result;
}

}
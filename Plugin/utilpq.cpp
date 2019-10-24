#include "utilpq.h"

#include <vtkSMProxy.h>
#include <vtkSMProperty.h>
#include <vtkSMInputProperty.h>
#include <vtkSMPropertyHelper.h>

#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqOutputPort.h>
#include <pqApplicationCore.h>

namespace utilpq {

bool multiple_inputs(pqPipelineFilter* filter, int port) {
  QString input_name = filter->getInputPortName(port);
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
      filter->getProxy()->GetProperty(input_name.toLocal8Bit().data()));
  return ip->GetMultipleInput();
}

bool can_connect(pqPipelineSource* source, int out_port, pqPipelineFilter* dest, int in_port) {
  return true;
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

}
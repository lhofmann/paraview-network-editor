#include "vtkPasteProxyLocator.h"
#include "vtkPasteStateLoader.h"

#include <vtkObjectFactory.h>
#include <vtkSMProxy.h>
#include <pqApplicationCore.h>
#include <pqServerManagerModel.h>
#include <pqProxy.h>
#include <pqServer.h>
#include <pqView.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkCollection.h>
#include <vtkSMViewProxy.h>
#include <vtkSMProxyProperty.h>
#include <vtkPVXMLElement.h>
#include <pqPipelineSource.h>

#include <vtkLogger.h>
#include <string>

namespace ParaViewNetworkEditor {

vtkStandardNewMacro(vtkPasteProxyLocator);

vtkPasteProxyLocator::vtkPasteProxyLocator() = default;

vtkPasteProxyLocator::~vtkPasteProxyLocator() = default;

void vtkPasteProxyLocator::PrintSelf(ostream &os, vtkIndent indent) {
  this->Superclass::PrintSelf(os, indent);
}

vtkSMProxy* vtkPasteProxyLocator::LocateProxy(vtkTypeUInt32 globalID) {
  vtkSMProxy* result = nullptr;
  if (!proxy_map.empty()) {
    auto it = proxy_map.find(globalID);
    if (it != proxy_map.end()) {
      vtkLog(5, "Found proxy from map: " << globalID);
      result = it->second;
    }
  }
  if (!result && FindExistingSources) {
    if (auto
        source = pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource *>(globalID)) {
      vtkLog(5, "Found proxy from pipeline: " << globalID);
      result = source->getProxy();
    }
  }

  return result ? result : this->Superclass::LocateProxy(globalID);
}

void vtkPasteProxyLocator::SetFindExistingSources(bool find_existing) {
  this->FindExistingSources = find_existing;
}

void vtkPasteProxyLocator::SetProxyMap(const std::map<vtkTypeUInt32, vtkSMProxy*>& map) {
  this->proxy_map = map;
}

}

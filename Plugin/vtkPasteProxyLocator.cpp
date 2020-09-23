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
  vtkSMProxy* result = this->Superclass::LocateProxy(globalID);
  if (!result) {
    if (FindExistingSources) {
      if (auto
          source = pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource *>(globalID)) {
        result = source->getProxy();
      }
    }
  }
  return result;
}

void vtkPasteProxyLocator::SetFindExistingSources(bool find_existing) {
  this->FindExistingSources = find_existing;
}

}

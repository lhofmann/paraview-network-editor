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

#include "debug_message.h"


namespace ParaViewNetworkEditor {

vtkStandardNewMacro(vtkPasteStateLoader);

vtkPasteStateLoader::vtkPasteStateLoader() = default;

vtkPasteStateLoader::~vtkPasteStateLoader() = default;

vtkSMProxy *vtkPasteStateLoader::NewProxy(vtkTypeUInt32 id, vtkSMProxyLocator *locator) {
  vtkSMProxy *result = this->Superclass::NewProxy(id, locator);
  if (result) {
    DEBUG_MSG(result->GetXMLName() << " " << result->GetXMLGroup() << " " << result->GetGlobalID());
    if (result->GetXMLGroup() == std::string("representations")) {
      representation_proxies.push_back(result);
    }
  }
  return result;
}

void vtkPasteStateLoader::PrintSelf(ostream &os, vtkIndent indent) {
  this->Superclass::PrintSelf(os, indent);
}

}

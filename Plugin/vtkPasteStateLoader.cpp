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

#include <vtkLogger.h>
#include <string>

namespace ParaViewNetworkEditor {

vtkStandardNewMacro(vtkPasteStateLoader);

vtkPasteStateLoader::vtkPasteStateLoader() = default;

vtkPasteStateLoader::~vtkPasteStateLoader() = default;

vtkSMProxy *vtkPasteStateLoader::NewProxy(vtkTypeUInt32 id, vtkSMProxyLocator *locator) {
  vtkPVXMLElement* elem = this->LocateProxyElement(id);
  if (!elem) {
    return nullptr;
  }
  auto annotations = vtkSmartPointer<vtkCollection>::New();
  elem->GetElementsByName("Annotation", annotations);
  std::string view_name;
  for (int i = 0; i < annotations->GetNumberOfItems(); ++i) {
    auto annotation = vtkPVXMLElement::SafeDownCast(annotations->GetItemAsObject(i));
    if (accept_active_view) {
      if (std::string(annotation->GetAttributeOrEmpty("key")) == "ActiveView") {
        std::string is_active_view = annotation->GetAttributeOrEmpty("value");
        if (is_active_view != "1") {
          return nullptr;
        }
      }
    }
    if (std::string(annotation->GetAttributeOrEmpty("key")) == "View") {
      view_name = annotation->GetAttributeOrEmpty("value");
      if (!accept_active_view) {
        if (accept_views.empty())
          return nullptr;
        if (std::find(accept_views.begin(), accept_views.end(), view_name) == accept_views.end())
          return nullptr;
      }
    }
  }

  vtkSMProxy *result = this->Superclass::NewProxy(id, locator);
  if (result) {
    if (result->GetXMLGroup() == std::string("representations")) {
      representation_proxies.emplace_back(std::make_tuple(view_name, result));
    }
  }
  return result;
}

void vtkPasteStateLoader::PrintSelf(ostream &os, vtkIndent indent) {
  this->Superclass::PrintSelf(os, indent);
}

}

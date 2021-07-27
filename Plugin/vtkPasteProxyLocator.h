#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_VTKPASTEPROXYLOCATOR_H
#define PARAVIEWNETWORKEDITOR_PLUGIN_VTKPASTEPROXYLOCATOR_H

#include <vtkSMProxyLocator.h>
#include <vtkType.h>
#include <map>

class vtkSMProxy;

namespace ParaViewNetworkEditor {

class vtkPasteProxyLocator : public vtkSMProxyLocator {
 public:
  static vtkPasteProxyLocator *New();
  vtkTypeMacro(vtkPasteProxyLocator, vtkSMProxyLocator);
  void PrintSelf(ostream &os, vtkIndent indent) override;

  vtkSMProxy* LocateProxy(vtkTypeUInt32 globalID) override;

  void SetFindExistingSources(bool);
  void SetProxyMap(const std::map<vtkTypeUInt32, vtkSMProxy*>&);

 protected:
  vtkPasteProxyLocator();
  ~vtkPasteProxyLocator() override;

  std::map<vtkTypeUInt32, vtkSMProxy*> proxy_map;

  bool FindExistingSources {false};

 private:
  vtkPasteProxyLocator(const vtkPasteProxyLocator &) = delete;
  void operator=(const vtkPasteProxyLocator &) = delete;
};

}

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_VTKPASTEPROXYLOCATOR_H

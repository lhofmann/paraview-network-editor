#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_VTKPASTESTATELOADER_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_VTKPASTESTATELOADER_H_

#include <vtkSMStateLoader.h>
#include <vector>

class vtkSMProxy;

class vtkPasteStateLoader : public vtkSMStateLoader {
 public:
  static vtkPasteStateLoader* New();
  vtkTypeMacro(vtkPasteStateLoader, vtkSMStateLoader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  std::vector<vtkSMProxy*> representation_proxies;
 protected:
  vtkPasteStateLoader();
  ~vtkPasteStateLoader() override;

  vtkSMProxy* NewProxy(vtkTypeUInt32 id, vtkSMProxyLocator* locator) override;

 private:
  vtkPasteStateLoader(const vtkPasteStateLoader&) = delete;
  void operator=(const vtkPasteStateLoader&) = delete;
};

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_VTKPASTESTATELOADER_H_

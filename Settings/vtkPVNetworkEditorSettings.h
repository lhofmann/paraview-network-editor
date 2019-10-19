#ifndef PARAVIEWNETWORKEDITOR_SETTINGS_VTKPVNETWORKEDITORSETTINGS_H_
#define PARAVIEWNETWORKEDITOR_SETTINGS_VTKPVNETWORKEDITORSETTINGS_H_

#include "NetworkEditorSettingsModule.h"
#include <vtkObject.h>
#include <vtkSmartPointer.h>

class NETWORKEDITORSETTINGS_EXPORT vtkPVNetworkEditorSettings : public vtkObject {
 public:
  static vtkPVNetworkEditorSettings *New();
  vtkTypeMacro(vtkPVNetworkEditorSettings, vtkObject);

  static vtkPVNetworkEditorSettings *GetInstance();

  vtkSetMacro(SwapOnStartup, bool);
  vtkGetMacro(SwapOnStartup, bool);

  vtkSetMacro(UpdateActiveObject, bool);
  vtkGetMacro(UpdateActiveObject, bool);

  vtkSetMacro(TooltipWakeupDelay, int);
  vtkGetMacro(TooltipWakeupDelay, int);

 protected:
  bool SwapOnStartup {true};
  bool UpdateActiveObject {true};
  int TooltipWakeupDelay {700};

  vtkPVNetworkEditorSettings();
  ~vtkPVNetworkEditorSettings() override;

 private:
  vtkPVNetworkEditorSettings(const vtkPVNetworkEditorSettings&) = delete;
  void operator=(const vtkPVNetworkEditorSettings&) = delete;

  static vtkSmartPointer<vtkPVNetworkEditorSettings> Instance;
};



#endif //PARAVIEWNETWORKEDITOR_SETTINGS_VTKPVNETWORKEDITORSETTINGS_H_

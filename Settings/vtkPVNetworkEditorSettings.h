#ifndef PARAVIEWNETWORKEDITOR_SETTINGS_VTKPVNETWORKEDITORSETTINGS_H_
#define PARAVIEWNETWORKEDITOR_SETTINGS_VTKPVNETWORKEDITORSETTINGS_H_

#include "NetworkEditorSettingsModule.h"
#include <vtkObject.h>
#include <vtkSmartPointer.h>
#include <string>

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

  vtkSetMacro(AutoSavePipelineScreenshot, bool);
  vtkGetMacro(AutoSavePipelineScreenshot, bool);

  vtkSetMacro(AutoSavePipelineSuffix, std::string);
  vtkGetMacro(AutoSavePipelineSuffix, std::string);

  vtkSetMacro(PipelineScreenshotTransparency, bool);
  vtkGetMacro(PipelineScreenshotTransparency, bool);

 protected:
  bool SwapOnStartup {false};
  bool UpdateActiveObject {true};
  int TooltipWakeupDelay {700};
  bool AutoSavePipelineScreenshot {true};
  bool PipelineScreenshotTransparency {true};
  std::string AutoSavePipelineSuffix {".pipeline.png"};

  vtkPVNetworkEditorSettings();
  ~vtkPVNetworkEditorSettings() override;

 private:
  vtkPVNetworkEditorSettings(const vtkPVNetworkEditorSettings&) = delete;
  void operator=(const vtkPVNetworkEditorSettings&) = delete;

  static vtkSmartPointer<vtkPVNetworkEditorSettings> Instance;
};



#endif //PARAVIEWNETWORKEDITOR_SETTINGS_VTKPVNETWORKEDITORSETTINGS_H_

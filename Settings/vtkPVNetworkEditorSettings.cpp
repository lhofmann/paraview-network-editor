#include "vtkPVNetworkEditorSettings.h"
#include <vtkObjectFactory.h>
#include <cassert>
#include <iostream>

vtkSmartPointer<vtkPVNetworkEditorSettings> vtkPVNetworkEditorSettings::Instance;

vtkPVNetworkEditorSettings* vtkPVNetworkEditorSettings::New() {
  vtkPVNetworkEditorSettings* instance = vtkPVNetworkEditorSettings::GetInstance();
  assert(instance);
  instance->Register(nullptr);
  return instance;
}

vtkPVNetworkEditorSettings* vtkPVNetworkEditorSettings::GetInstance() {
  if (!vtkPVNetworkEditorSettings::Instance) {
    vtkPVNetworkEditorSettings* instance = new vtkPVNetworkEditorSettings();
    instance->InitializeObjectBase();
    vtkPVNetworkEditorSettings::Instance.TakeReference(instance);
  }
  return vtkPVNetworkEditorSettings::Instance;
}

vtkPVNetworkEditorSettings::vtkPVNetworkEditorSettings() = default;

vtkPVNetworkEditorSettings::~vtkPVNetworkEditorSettings() = default;

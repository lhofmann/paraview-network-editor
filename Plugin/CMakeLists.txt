find_package(Qt5 REQUIRED COMPONENTS Widgets)

set(interfaces)
set(sources
    NetworkEditorWidget.cpp
    NetworkEditorWidget.h
    NetworkEditor.cpp
    NetworkEditor.h
    NetworkEditorView.cpp
    NetworkEditorView.h
    utilqt.cpp)

paraview_plugin_add_dock_window(
  CLASS_NAME NetworkEditorWidget
  DOCK_AREA Right
  INTERFACES dock_interfaces
  SOURCES dock_sources)
list(APPEND interfaces
  ${dock_interfaces})
list(APPEND sources
  ${dock_sources})

paraview_add_plugin(NetworkEditor
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})

target_link_libraries(NetworkEditor
  PRIVATE
    Qt5::Widgets)

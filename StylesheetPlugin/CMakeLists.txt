set(interfaces)
set(resources resources.qrc)
set(sources
    LoadStylesheet.cxx
    LoadStylesheet.h)

paraview_plugin_add_auto_start(
  CLASS_NAME LoadStylesheet
  STARTUP onStartup
  SHUTDOWN onShutdown
  INTERFACES autostart_interface
  SOURCES autostart_sources)
list(APPEND interfaces
  ${autostart_interface})
list(APPEND sources
  ${autostart_sources})

paraview_add_plugin(LoadStylesheet
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  UI_RESOURCES ${resources}
  SOURCES ${sources})

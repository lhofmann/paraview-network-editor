cmake_minimum_required(VERSION 3.8)
project(ParaViewNetworkEditor)

find_package(ParaView REQUIRED)

set(BUILD_SHARED_LIBS ON)

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

paraview_plugin_scan(
  PLUGIN_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Plugin/paraview.plugin"
                    "${CMAKE_CURRENT_SOURCE_DIR}/StylesheetPlugin/paraview.plugin"
  PROVIDES_PLUGINS  plugins
  REQUIRES_MODULES  required_modules
  ENABLE_BY_DEFAULT ON)

foreach (module IN LISTS required_modules)
  if (NOT TARGET "${module}")
    message("Missing required module: ${module}")
    return ()
  endif ()
endforeach ()

paraview_plugin_build(
  RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY_SUBDIRECTORY "${PARAVIEW_PLUGIN_SUBDIR}"
  PLUGINS ${plugins})

set(_plugin_config_file "")
foreach(plugin ${plugins})
  set(_plugin_config_file "${_plugin_config_file}\n<Plugin name=\"${plugin}\" filename=\"$<TARGET_FILE:${plugin}>\"/>")
endforeach()
set(_plugin_config_file "<?xml version=\"1.0\"?>\n<Plugins>\n${_plugin_config_file}\n</Plugins>\n")
file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/plugin_config.xml.in"
    CONTENT "${_plugin_config_file}")
file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/plugin_config.xml"
    INPUT "${CMAKE_BINARY_DIR}/plugin_config.xml.in")
if (VTK_PREFIX_PATH)
  set(_paraview_bin_path "${VTK_PREFIX_PATH}/bin")
elseif (VTK_INSTALL_PREFIX)
  set(_paraview_bin_path "${VTK_INSTALL_PREFIX}/bin")
else ()
  set(_paraview_bin_path "${ParaView_DIR}/bin")
endif ()
set(_paraview_plugin_config_file "${CMAKE_BINARY_DIR}/plugin_config.xml")
configure_file("paraview.sh.in" "${CMAKE_BINARY_DIR}/paraview.sh" @ONLY)

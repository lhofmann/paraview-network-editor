cmake_minimum_required(VERSION 3.8)
project(ParaViewNetworkEditor)

include(find_graphviz.cmake)

find_package(ParaView REQUIRED)

if (ParaView_VERSION VERSION_LESS 5.7)
  set(legacy_build_system TRUE)
else ()
  set(legacy_build_system FALSE)
endif ()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall)

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

if (legacy_build_system)
  include(GenerateExportHeader)
  include(${ParaView_USE_FILE})

  if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
  endif()
  set(CMAKE_AUTOMOC TRUE)

  add_subdirectory(Plugin)
  set(plugins NetworkEditor)
else ()
  set(BUILD_SHARED_LIBS ON)
  vtk_module_scan(
      MODULE_FILES         "${CMAKE_CURRENT_SOURCE_DIR}/Settings/vtk.module"
      PROVIDES_MODULES     modules
      REQUIRES_MODULES     required_modules
      UNRECOGNIZED_MODULES unrecognized_modules
      WANT_BY_DEFAULT      ON)
  vtk_module_build(
      MODULES             ${modules}
      INSTALL_HEADERS     OFF)

  paraview_plugin_scan(
    PLUGIN_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Plugin/paraview.plugin"
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
    RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}/plugins"
    LIBRARY_DESTINATION "${CMAKE_INSTALL_BINDIR}/plugins"
    ADD_INSTALL_RPATHS ON
    PLUGINS ${plugins})
endif ()

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

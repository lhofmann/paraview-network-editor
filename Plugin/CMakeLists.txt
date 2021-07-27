find_package(Qt5 REQUIRED COMPONENTS Widgets)

set(sources
    NetworkEditorWidget.cpp
    NetworkEditorWidget.h
    NetworkEditor.cpp
    NetworkEditor.h
    NetworkEditorView.cpp
    NetworkEditorView.h
    EditorGraphicsItem.cpp
    SourceGraphicsItem.cpp
    LabelGraphicsItem.cpp
    PortGraphicsItem.cpp
    ConnectionGraphicsItem.cpp
    ConnectionDragHelper.cpp
    OutputPortStatusGraphicsItem.cpp
    StickyNoteGraphicsItem.cpp
    utilqt.cpp
    utilpq.cpp
    observer.cpp
    vtkPasteProxyLocator.cpp
    vtkPasteStateLoader.cpp)


if (legacy_build_system)
    add_paraview_dock_window(
        interfaces
        dock_sources
        CLASS_NAME NetworkEditorWidget
        DOCK_AREA Right)
    list(APPEND sources
        ${dock_sources})

    set(settings_dir "${CMAKE_CURRENT_LIST_DIR}/../Settings")
    set(settings_sources
        "${settings_dir}/vtkPVNetworkEditorSettings.cpp"
        "${settings_dir}/vtkPVNetworkEditorSettings.h")

    add_paraview_plugin(NetworkEditor "1.0"
        SERVER_MANAGER_XML Settings.xml
        SERVER_MANAGER_SOURCES ${settings_sources}
        GUI_INTERFACES ${interfaces}
        GUI_SOURCES ${sources})
    set_target_properties(NetworkEditor PROPERTIES PREFIX "")
    target_include_directories(NetworkEditor PUBLIC ${settings_dir})

    generate_export_header(NetworkEditor
        EXPORT_MACRO_NAME "NETWORKEDITORSETTINGS_EXPORT"
        EXPORT_FILE_NAME "NetworkEditorSettingsModule.h")
    target_include_directories(NetworkEditor PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
else ()
    paraview_plugin_add_dock_window(
      CLASS_NAME NetworkEditorWidget
      DOCK_AREA Right
      INTERFACES interfaces
      SOURCES dock_sources)
    list(APPEND sources
      ${dock_sources})

    paraview_add_plugin(NetworkEditor
      VERSION "1.0"
      UI_INTERFACES ${interfaces}
      UI_RESOURCES ${resources}
      SOURCES ${sources}
      SERVER_MANAGER_XML Settings.xml
      XML_DOCUMENTATION OFF
      MODULES NetworkEditorSettings)

    set_target_properties(NetworkEditor PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
    set_target_properties(NetworkEditor PROPERTIES LINK_FLAGS "-Wl,--no-undefined")

    target_link_libraries(NetworkEditor
      PRIVATE
        Qt5::Widgets
        ParaView::pqComponents
        ParaView::pqApplicationComponents
        ParaView::RemotingViews
        #ParaView::ServerManagerRendering
    )
endif ()

if (ENABLE_GRAPHVIZ)
    target_sources(NetworkEditor PRIVATE graph_layout.cpp)
    target_compile_definitions(NetworkEditor PRIVATE ENABLE_GRAPHVIZ)
    target_link_libraries(NetworkEditor PRIVATE GraphViz)
endif ()

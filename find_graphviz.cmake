# borrowed from https://github.com/topology-tool-kit/ttk

find_path(GRAPHVIZ_INCLUDE_DIR
    NAMES
    graphviz/cgraph.h
    HINTS
    ${_GRAPHVIZ_INCLUDE_DIR}
    )

find_library(GRAPHVIZ_CDT_LIBRARY
    NAMES
    cdt
    HINTS
    ${_GRAPHVIZ_LIBRARY_DIR}
    )

find_library(GRAPHVIZ_GVC_LIBRARY
    NAMES
    gvc
    HINTS
    ${_GRAPHVIZ_LIBRARY_DIR}
    )

find_library(GRAPHVIZ_CGRAPH_LIBRARY
    NAMES
    cgraph
    HINTS
    ${_GRAPHVIZ_LIBRARY_DIR}
    )

find_library(GRAPHVIZ_PATHPLAN_LIBRARY
    NAMES
    pathplan
    HINTS
    ${_GRAPHVIZ_LIBRARY_DIR}
    )

if (GRAPHVIZ_GVC_LIBRARY)
    get_filename_component(_GRAPHVIZ_ROOT "${GRAPHVIZ_GVC_LIBRARY}" DIRECTORY)
else ()
    set(_GRAPHVIZ_ROOT ${_GRAPHVIZ_LIBRARY_DIR})
endif()

find_library(GRAPHVIZ_GVPLUGIN_DOT_LAYOUT
    NAMES
    gvplugin_dot_layout
    HINTS
    ${_GRAPHVIZ_ROOT}/graphviz
    )

find_library(GRAPHVIZ_GVPLUGIN_CORE
    NAMES
    gvplugin_core
    HINTS
    ${_GRAPHVIZ_ROOT}/graphviz
    )


if( GRAPHVIZ_INCLUDE_DIR
    AND GRAPHVIZ_GVPLUGIN_DOT_LAYOUT
    AND GRAPHVIZ_GVPLUGIN_CORE
    AND GRAPHVIZ_CDT_LIBRARY
    AND GRAPHVIZ_GVC_LIBRARY
    AND GRAPHVIZ_CGRAPH_LIBRARY
    AND GRAPHVIZ_PATHPLAN_LIBRARY
    )
    set(GRAPHVIZ_FOUND "YES")
else()
    set(GRAPHVIZ_FOUND "NO")
endif()

if(GRAPHVIZ_FOUND)
    find_package(ZLIB)
    if (NOT ZLIB_FOUND)
        message(WARNING "GraphViz found, but missing dependency ZLIB. Disabling GraphViz support.")
        set(GRAPHVIZ_FOUND "NO")
    endif()
endif()

if(GRAPHVIZ_FOUND)
    option(ENABLE_GRAPHVIZ "Enable GraphViz support" ON)
    message(STATUS "GraphViz found")
    add_library(GraphViz INTERFACE IMPORTED)
    target_link_libraries(GraphViz INTERFACE ZLIB::ZLIB)
    target_include_directories(GraphViz INTERFACE ${GRAPHVIZ_INCLUDE_DIR})
    target_link_libraries(GraphViz INTERFACE
        ${GRAPHVIZ_GVPLUGIN_DOT_LAYOUT}
        ${GRAPHVIZ_GVPLUGIN_CORE}
        ${GRAPHVIZ_GVC_LIBRARY}
        ${GRAPHVIZ_PATHPLAN_LIBRARY}
        ${GRAPHVIZ_CGRAPH_LIBRARY}
        ${GRAPHVIZ_CDT_LIBRARY})
else()
    option(ENABLE_GRAPHVIZ "Enable GraphViz support" OFF)
    message(STATUS "GraphViz not found, disabling GraphViz support.")
endif()

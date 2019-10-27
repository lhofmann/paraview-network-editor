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

if( GRAPHVIZ_INCLUDE_DIR
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
    option(ENABLE_GRAPHVIZ "Enable GraphViz support" ON)
    message(STATUS "GraphViz found")
else()
    option(ENABLE_GRAPHVIZ "Enable GraphViz support" OFF)
    message(STATUS "GraphViz not found, disabling GraphViz support")
endif()


# FindGraphViz
# Try to find GraphViz library
#
# This module defines the following variables:
#
#  GRAPHVIZ_FOUND          - system has Graphviz
#  GRAPHVIZ_INCLUDE_DIRS   - Graphviz include directories
#  GRAPHVIZ_LIBRARIES      - all GraphViz libraries
#  GRAPHVIZ_CDT_LIBRARY    - Graphviz CDT library
#  GRAPHVIZ_GVC_LIBRARY    - Graphviz GVC library
#  GRAPHVIZ_CGRAPH_LIBRARY - Graphviz CGRAPH library
#
# and the following imported targets:
#   GRAPHVIZ::GRAPHVIZ - The core DUMPI library
#
# You can also set the following variable to help guide the search:
#   GRAPHVIZ_ROOT - The install prefix for GraphViz containing the
#                     include and lib folders
#                     Note: this can be set as a CMake variable or an
#                           environment variable.  If specified as a CMake
#                           variable, it will override any setting specified
#                           as an environment variable.

if ((NOT GRAPHVIZ_ROOT) AND (NOT (ENV{GRAPHVIZ_ROOT} STREQUAL "")))
  set(GRAPHVIZ_ROOT "$ENV{GRAPHVIZ_ROOT}")
endif()
if(GRAPHVIZ_ROOT)
  set(_GRAPHVIZ_INCLUDE_DIR ${GRAPHVIZ_ROOT}/include)
  set(_GRAPHVIZ_LIBRARY_DIR ${GRAPHVIZ_ROOT}/lib)
endif()

find_path(GRAPHVIZ_INCLUDE_DIR         NAMES graphviz/cgraph.h
          HINTS ${_GRAPHVIZ_INCLUDE_DIR})
find_library(GRAPHVIZ_CDT_LIBRARY      NAMES cdt
             HINTS ${_GRAPHVIZ_LIBRARY_DIR})
find_library(GRAPHVIZ_GVC_LIBRARY      NAMES gvc
             HINTS ${_GRAPHVIZ_LIBRARY_DIR})
find_library(GRAPHVIZ_CGRAPH_LIBRARY   NAMES cgraph
             HINTS ${_GRAPHVIZ_LIBRARY_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GraphViz
  FOUND_VAR GRAPHVIZ_FOUND
  REQUIRED_VARS GRAPHVIZ_INCLUDE_DIR GRAPHVIZ_CDT_LIBRARY GRAPHVIZ_GVC_LIBRARY GRAPHVIZ_CGRAPH_LIBRARY
)

if(GRAPHVIZ_FOUND)
  set(GRAPHVIZ_INCLUDE_DIRS ${GRAPHVIZ_INCLUDE_DIR})
  set(GRAPHVIZ_LIBRARIES ${GRAPHVIZ_CDT_LIBRARY} ${GRAPHVIZ_GVC_LIBRARY} ${GRAPHVIZ_CGRAPH_LIBRARY})
  message(STATUS "GraphViz Libraries \"${GRAPHVIZ_LIBRARIES}\"")
  if(NOT TARGET GraphViz::cdt)
    add_library(GraphViz::cdt UNKNOWN IMPORTED)
    set_target_properties(GraphViz::cdt PROPERTIES
        IMPORTED_LOCATION             "${GRAPHVIZ_CDT_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${GRAPHVIZ_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES      "${GRAPHVIZ_CDT_LIBRARY}"
    )
  endif()
  if(NOT TARGET GraphViz::gvc)
    add_library(GraphViz::gvc UNKNOWN IMPORTED)
    set_target_properties(GraphViz::gvc PROPERTIES
        IMPORTED_LOCATION             "${GRAPHVIZ_GVC_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${GRAPHVIZ_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES      "${GRAPHVIZ_LIBRARIES}"
    )
  endif()
  if(NOT TARGET GraphViz::cgraph)
    add_library(GraphViz::cgraph UNKNOWN IMPORTED)
    set_target_properties(GraphViz::cgraph PROPERTIES
        IMPORTED_LOCATION             "${GRAPHVIZ_CGRAPH_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${GRAPHVIZ_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES      "${GRAPHVIZ_LIBRARIES}"
    )
  endif()
endif()

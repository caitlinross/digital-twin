# FindDUMPI
# -----------
#
# Try to find the SST DUMPI trace reader library
#
# This module defines the following variables:
#
#   DUMPI_FOUND        - System has DUMPI
#   DUMPI_INCLUDE_DIRS - The DUMPI include directory
#   DUMPI_LIBRARIES    - Link these to use DUMPI
#
# and the following imported targets:
#   DUMPI::DUMPI - The core DUMPI library
#
# You can also set the following variable to help guide the search:
#   DUMPI_ROOT - The install prefix for DUMPI containing the
#                     include and lib folders
#                     Note: this can be set as a CMake variable or an
#                           environment variable.  If specified as a CMake
#                           variable, it will override any setting specified
#                           as an environment variable.

if((NOT DUMPI_ROOT) AND (NOT (ENV{DUMPI_ROOT} STREQUAL "")))
  set(DUMPI_ROOT "$ENV{DUMPI_ROOT}")
endif()
if(DUMPI_ROOT)
  set(DUMPI_INCLUDE_OPTS HINTS ${DUMPI_ROOT}/include NO_DEFAULT_PATHS)
  set(DUMPI_LIBRARY_OPTS
    HINTS ${DUMPI_ROOT}/lib
    NO_DEFAULT_PATHS
    )
endif()

find_path(DUMPI_INCLUDE_DIR dumpi/libundumpi/libundumpi.h ${DUMPI_INCLUDE_OPTS})
find_library(DUMPI_LIBRARY NAMES undumpi ${DUMPI_LIBRARY_OPTS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DUMPI
  FOUND_VAR DUMPI_FOUND
  REQUIRED_VARS DUMPI_LIBRARY DUMPI_INCLUDE_DIR
)

cmake_print_variables(DUMPI_INCLUDE_DIR)
if(DUMPI_FOUND)
  set(DUMPI_INCLUDE_DIRS ${DUMPI_INCLUDE_DIR})
  set(DUMPI_LIBRARIES ${DUMPI_LIBRARY})
  message(STATUS "DUMPI Libraries \"${DUMPI_LIBRARIES}\"")
  if(NOT TARGET DUMPI::DUMPI)
    add_library(DUMPI::DUMPI UNKNOWN IMPORTED)
    set_target_properties(DUMPI::DUMPI PROPERTIES
      IMPORTED_LOCATION             "${DUMPI_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${DUMPI_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES      "${DUMPI_LIBRARIES}"
    )
  endif()
endif()

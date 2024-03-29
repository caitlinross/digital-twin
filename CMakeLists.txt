cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(CODES LANGUAGES C CXX VERSION 0.1)

include(CMakePrintHelpers)
cmake_print_variables(CMAKE_CURRENT_SOURCE_DIR)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# version info
include(Utilities/Git/Git.cmake)
include(CodesDTDetermineVersion)

## load hardcoded version in case this is not a Git repo
file(STRINGS version.txt version_txt)
extract_version_components("${version_txt}" "CODES_DT")
# Get version from Git if we can
determine_version(${PROJECT_SOURCE_DIR} ${GIT_EXECUTABLE} "CODES_DT")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

option(CODES_ENABLE_TESTING "Enable testing" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

#prevent cmake from stripping the runtime path (important if shared libraries are imported)
#SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

find_package(ROSS REQUIRED)

find_package(GraphViz REQUIRED)

#set(SWM_PKG_CONFIG_PATH "" CACHE PATH "Where is the SWM PKG_CONFIG installed?")
#set(ARGOBOTS_PKG_CONFIG_PATH "" CACHE PATH "Where is argobots PKG_COPNFIG installed? Necessary for SWM")
#
#find_package(PkgConfig REQUIRED)
#set(ENV{PKG_CONFIG_PATH} "${ROSS_PKG_CONFIG_PATH}:${SWM_PKG_CONFIG_PATH}:${ARGOBOTS_PKG_CONFIG_PATH}")
#pkg_check_modules(ROSS REQUIRED IMPORTED_TARGET ross)

# MPI
include(SetupMPI)
if(MPI_C_FOUND)
        include_directories(${MPI_C_INCLUDE_PATH})
        list(APPEND CODES_EXTERNAL_LIBS ${MPI_C_LIBRARIES})
else(MPI_C_FOUND)
        message("WARNING: Could not find MPI!")
        message("         Either add an MPI compiler to your path (using modules)")
        message("         Or force CMake to build using the correct compiler (`export CC=mpicc`)")
endif(MPI_C_FOUND)


## DUMPI
set(CODES_HAVE_DUMPI FALSE)
find_package(DUMPI)
if (DUMPI_FOUND)
  set(CODES_HAVE_DUMPI TRUE)
endif()

## libtorch
set(CODES_HAVE_TORCH FALSE)
find_package(Torch)
if (TORCH_FOUND)
  set(CODES_HAVE_TORCH TRUE)
endif()

add_subdirectory(codes)

### SWM
#pkg_check_modules(SWM IMPORTED_TARGET swm)
#if(NOT SWM_FOUND)
#        message(STATUS "SWM Library Not Found, Online workloads disabled")
#else(SWM_FOUND)
#        message(STATUS "SWM Library Found: ${SWM_LIBRARIES}")
#        pkg_check_modules(ARGOBOTS REQUIRED IMPORTED_TARGET argobots)
#        if(NOT ARGOBOTS_FOUND)
#                message(STATUS "Argobots Library Not Found, Online workloads disabled")
#        else(ARGOBOTS_FOUND)
#                message(STATUS "Argobots Library Found: ${ARGOBOTS_LIBRARIES}")
#                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARGOBOTS_CFLAGS} -I${ARGOBOTS_INCLUDE}")
#
#                pkg_get_variable(SWM_DATAROOTDIR swm datarootdir)
#                cmake_print_variables(SWM_DATAROOTDIR)
#        
#                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SWM_CFLAGS} -I${SWM_INCLUDE}")
#                add_definitions(-DUSE_ONLINE=1)
#                set(USE_ONLINE true)
#        endif()
#endif()

if (CODES_ENABLE_TESTING)
  enable_testing()
  include(CTest)
  add_subdirectory(tests)
endif()

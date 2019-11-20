cmake_minimum_required(VERSION 3.1)

project(dhyana)

# Include additional modules that are used inconditionnaly
include(GNUInstallDirs)
include(GenerateExportHeader)

# If conda build, always set lib dir to 'lib'
if($ENV{CONDA_BUILD})
  set(CMAKE_INSTALL_LIBDIR "lib")
endif()

# Set lower / upper case project names
string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_UPPER)
string(TOLOWER "${PROJECT_NAME}" PROJECT_NAME_LOWER)

# Set targets export name (used by lima and dependencies)
set(TARGETS_EXPORT_NAME "${PROJECT_NAME_LOWER}-targets")
#set(namespace "${PROJECT_NAME}::")

# Check if project is being used directly or via add_subdirectory
set(CAMERA_MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(CAMERA_MASTER_PROJECT ON)
endif()

if (CAMERA_MASTER_PROJECT)
  find_package(Lima REQUIRED COMPONENTS devel tools)
endif()

# CMake additional macros
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${LIMA_CMAKE_INCLUDE_DIRS} ${CMAKE_MODULE_PATH})

include(LimaTools)

# Set version
include(project_version)

# Enable python binding code compilation using sip generator
if (CAMERA_MASTER_PROJECT)
  option(LIMA_ENABLE_PYTHON "compile python binding code?" OFF)
  option(LIMA_ENABLE_PYTANGO_SERVER "install python tango server?" OFF)

  if (LIMA_ENABLE_PYTHON)
    limatools_find_python_and_sip()
  endif()
endif()

#find_package(LIBTUCAM REQUIRED)

file(GLOB_RECURSE DHYANA_INCS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE TUCAM_INCS "${CMAKE_CURRENT_SOURCE_DIR}/sdk/include/*.h")

# Library definition
add_library(dhyana SHARED
  src/DhyanaCamera.cpp
  src/DhyanaInterface.cpp
  src/DhyanaDetInfoCtrlObj.cpp
  src/DhyanaSyncCtrlObj.cpp
  src/DhyanaRoiCtrlObj.cpp
  src/DhyanaBinCtrlObj.cpp
  src/DhyanaTimer.cpp
  ${DHYANA_INCS}
  ${TUCAM_INCS}
)

# Generate export macros
generate_export_header(dhyana)

# Set version and output name
set_target_properties(dhyana PROPERTIES
  OUTPUT_NAME "lima${PROJECT_NAME_LOWER}"
  VERSION "${PROJECT_VERSION}"
  SOVERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}")

# Add dependencies
target_include_directories(dhyana
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sdk/include>"
    4PUBLIC "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(dhyana PUBLIC limacore)

# Binding code for python
if(LIMA_ENABLE_PYTHON)
  limatools_run_sip_for_camera(dhyana)
endif()

# Generate and install package config file and version
if(CAMERA_MASTER_PROJECT)
    set (PROJECT_LIBRARIES dhyana)
    set(SIP_INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/sip/lima)
    set(CMAKE_INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/cmake/lima)
    include(package_config)
endif()

## Installation

install(
  TARGETS dhyana
  EXPORT "${TARGETS_EXPORT_NAME}"
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}   # import library
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}   # .so files are libraries
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}   # .dll files are binaries
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}  # this does not actually install anything (but used by downstream projects)
)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  COMPONENT devel
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h"
)

# if(LIMA_ENABLE_PYTHON)
#   install(
#     DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python/
#     DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/Basler"
#     )
#   if (LIMA_ENABLE_PYTANGO_SERVER)
#     add_subdirectory(tango)
#   endif()
# endif()
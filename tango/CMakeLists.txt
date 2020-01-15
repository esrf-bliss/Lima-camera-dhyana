cmake_minimum_required(VERSION 3.1)

project(dhyana_tango NONE)

# use lima toolbox function,
# if cmake is ran out of camera mother project, check python environment  
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  find_package(Lima REQUIRED COMPONENTS tools)
  set(CMAKE_MODULE_PATH
    ${LIMA_CMAKE_INCLUDE_DIRS} ${CMAKE_MODULE_PATH})
  include(LimaTools)
endif()

limatools_install_camera_tango("Dhyana.py")
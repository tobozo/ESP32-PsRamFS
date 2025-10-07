# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.10)
# Safe to remove the next line if ESP32-PsRamFS is installed in the components/ folder of this project
list(APPEND EXTRA_COMPONENT_DIRS "$(PROJECT_PATH)/../../../../ESP32-PsRamFS")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(psramvfs)

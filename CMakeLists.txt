cmake_minimum_required(VERSION 3.6)

project(
        AEVRSSE
        VERSION 0.0.1
        LANGUAGES CXX)
set(PROJECT_LICENSE "MIT")
set(PROJECT_FRIENDLY_NAME ${PROJECT_NAME})
set(PROJECT_LONG_NAME "AEVRSSE CommonLibSSE-NG")
set(PROJECT_DESCRIPTION ${PROJECT_LONG_NAME})
set(PROJECT_COPYRIGHT "Copyright")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(GNUInstallDirs)
include(addpluginsources)
include(pluginconfig)
include(plugintarget)
include(pluginpackage)
include("${PROJECT_SOURCE_DIR}/local.cmake" OPTIONAL)

######## dependencies
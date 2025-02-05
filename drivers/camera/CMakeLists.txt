cmake_minimum_required(VERSION 3.15...3.30)

project(Camera
  VERSION 0.0.1
  LANGUAGES CXX
)

set(base_proj_path ${CMAKE_CURRENT_LIST_DIR}/../..)
include(${base_proj_path}/cmake/CPM.cmake)
CPMUsePackageLock(${base_proj_path}/package-lock.cmake)
CPMGetPackage(PackageProject.cmake)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GStreamer REQUIRED IMPORTED_TARGET gstreamer-1.0 gstreamer-app-1.0)

add_library(${PROJECT_NAME} cam.cc)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::GStreamer)
target_compile_options(${PROJECT_NAME} PRIVATE -std=c++17 -Wall -Wextra -Wpedantic)
target_include_directories(${PROJECT_NAME}
  PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

add_executable(cam_run main.cc)
target_link_libraries(cam_run PRIVATE ${PROJECT_NAME} PkgConfig::GStreamer)
target_compile_options(cam_run PRIVATE -std=c++17)

string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)
packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  VERSION_HEADER "${VERSION_HEADER_LOCATION}"
  COMPATIBILITY SameMajorVersion
  DEPENDENCIES "PkgConfig::GStreamer"
)

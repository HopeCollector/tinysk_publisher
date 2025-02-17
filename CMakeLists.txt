cmake_minimum_required(VERSION 3.15...3.30)

# ---- Project ----
project(
  TSKPub
  VERSION 0.0.1
  LANGUAGES C CXX
)

# ---- Include guards ----
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
    "禁止在项目源码目录中构建，请在项目源码目录外新建一个目录（构建目录），然后在构建目录中运行 CMake。"
  )
endif()

# ---- Add dependencies via CPM ----
# 远程依赖
include(cmake/CPM.cmake)
CPMUsePackageLock(package-lock.cmake)
CPMGetPackage(PackageProject.cmake)
CPMGetPackage(spdlog)
CPMGetPackage(fkYAML)
CPMGetPackage(cppzmq)
set(dep_list "spdlog 1.8.5" "fkYAML 0.4.1" "cppzmq 4.10.0")

# 本地第三方依赖
set(driver_dir ${CMAKE_CURRENT_LIST_DIR}/drivers)
CPMAddPackage(NAME imu URL ${driver_dir}/imu.tar.gz)
CPMAddPackage(NAME Camera URL ${driver_dir}/camera.tar.gz)
CPMAddPackage(NAME lidar URL ${driver_dir}/lidar.tar.gz)
list(APPEND dep_list "imu" "Camera" "lidar")

# 本地系统依赖
find_package(CapnProto CONFIG REQUIRED)
find_package(PCL REQUIRED COMPONENTS common filters)
list(APPEND dep_list "CapnProto" "PCL COMPONENTS common filters")
message(STATUS "PCL_INCLUDE_DIRS: ${PCL_INCLUDE_DIRS}")
message(STATUS "PCL_LIBRARIES: ${PCL_LIBRARIES}")

# ---- Create library ----
add_subdirectory(messages)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -Wpedantic")
add_subdirectory(source)

# ---- Create an installable target ----
string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)
string(JOIN ";" dep_list "${dep_list}")

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  INCLUDE_HEADER_PATTERN "*.h;*.hh"
  VERSION_HEADER "${VERSION_HEADER_LOCATION}"
  COMPATIBILITY SameMajorVersion
  DEPENDENCIES "${dep_list}"
)

packageProject(
  NAME messages
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  DEPENDENCIES "CapnProto 1.1.0"
)

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
# 依赖管理
include(cmake/CPM.cmake)
CPMUsePackageLock(package-lock.cmake)
CPMGetPackage(PackageProject.cmake)
CPMGetPackage(spdlog)
CPMGetPackage(fkYAML)
CPMGetPackage(cppzmq)

# ---- Create library ----
add_library(${PROJECT_NAME} source/tskpub.cc)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
target_compile_options(${PROJECT_NAME} PRIVATE -std=c++17 -Wall -Wextra -Wpedantic)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog fkYAML::fkYAML cppzmq)
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# ---- Create an installable target ----
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
  DEPENDENCIES "spdlog 1.8.5;fkYAML 0.4.1;cppzmq 4.10.0"
)

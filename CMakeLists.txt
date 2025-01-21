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

# 3rdpart 安装目录
set(INSTALL_DIR_3RD ${CMAKE_BINARY_DIR}/3rdpart)
list(APPEND CMAKE_PREFIX_PATH ${INSTALL_DIR_3RD})

# 项目打包
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.13.0")

# 日志
CPMAddPackage(
  NAME spdlog
  VERSION 1.15.0
  GITHUB_REPOSITORY gabime/spdlog
  OPTIONS
    "SPDLOG_BUILD_EXAMPLE OFF"
    "SPDLOG_BUILD_TESTS OFF"
    "SPDLOG_INSTALL ON"
)

# 配置文件解析
CPMAddPackage(
  NAME fkYAML
  VERSION 0.4.1
  GITHUB_REPOSITORY fktn-k/fkYAML
  OPTIONS
    "FK_YAML_INSTALL ON"
)

# 通信
CPMAddPackage("gh:zeromq/cppzmq@4.10.0")

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
# this allows users to install and find the library via `find_package()`.

# the location where the project's version header will be placed should match the project's regular
# header paths
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
  DEPENDENCIES "spdlog::spdlog 1.8.5;fkYAML::fkYAML 0.4.1;cppzmq 4.10.0"
)

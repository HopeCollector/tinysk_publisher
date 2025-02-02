cmake_minimum_required(VERSION 3.15)

project(TSKPubTest LANGUAGES CXX)

# ---- Options ----

option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)
option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)
CPMUsePackageLock(${PROJECT_SOURCE_DIR}/../package-lock.cmake)
CPMGetPackage(spdlog)
CPMGetPackage(fkYAML)
CPMGetPackage(cppzmq)
CPMAddPackage("gh:doctest/doctest@2.4.11")
CPMAddPackage(
  NAME imu
  VERSION 1.0.0
  URL "file://${CMAKE_CURRENT_LIST_DIR}/../drivers/imu.tar.gz"
)

if(TEST_INSTALLED_VERSION)
  find_package(TSKPub REQUIRED)
else()
  CPMAddPackage(NAME TSKPub SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
endif()

# ---- Create binary ----
include(CTest)
enable_testing()

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cc)
file(GLOB_RECURSE lib_srcs CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../source/*.cc)
add_executable(${PROJECT_NAME} ${sources} ${lib_srcs})
target_link_libraries(${PROJECT_NAME} doctest::doctest TSKPub::messages imu spdlog fkYAML cppzmq)
target_include_directories(${PROJECT_NAME} PRIVATE 
  ${PROJECT_SOURCE_DIR}/../source
  ${PROJECT_SOURCE_DIR}/../include)
target_compile_options(${PROJECT_NAME} PRIVATE -std=c++17)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(${PROJECT_NAME})

# ---- code coverage ----

if(ENABLE_TEST_COVERAGE)
  target_compile_options(TSKPub PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
  target_link_options(TSKPub PUBLIC -fprofile-arcs -ftest-coverage)
endif()

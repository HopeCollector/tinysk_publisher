cmake_minimum_required(VERSION 3.15)
project(TSKPubIntegrationTests LANGUAGES CXX)

set(base_dir ${PROJECT_SOURCE_DIR}/../..)
include(${base_dir}/cmake/CPM.cmake)
CPMAddPackage(NAME TSKPub SOURCE_DIR ${base_dir})
CPMAddPackage("gh:doctest/doctest@2.4.11")
CPMUsePackageLock(${base_dir}/package-lock.cmake)
CPMGetPackage(fkYAML)

add_executable(${PROJECT_NAME} main.cc)
target_link_libraries(${PROJECT_NAME} PRIVATE TSKPub::TSKPub doctest fkYAML)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
configure_file(cfg.yml.hh.in ${CMAKE_CURRENT_BINARY_DIR}/cfg.yml.hh)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
enable_testing()
doctest_discover_tests(${PROJECT_NAME})

cmake_minimum_required(VERSION 3.15)

project(TSKPubUnitTest LANGUAGES CXX)

set(base_dir ${PROJECT_SOURCE_DIR}/../..)
set(src_dir ${base_dir}/source)
set(driver_dir ${base_dir}/drivers)

include(${base_dir}/cmake/CPM.cmake)
CPMAddPackage(NAME TSKPub SOURCE_DIR ${base_dir})
CPMUsePackageLock(${base_dir}/package-lock.cmake)
CPMGetPackage(spdlog)
CPMGetPackage(fkYAML)
CPMAddPackage(NAME imu URL ${driver_dir}/imu.tar.gz)
CPMAddPackage(NAME Camera URL ${driver_dir}/camera.tar.gz)
CPMAddPackage(NAME lidar URL ${driver_dir}/lidar.tar.gz)
CPMAddPackage("gh:doctest/doctest@2.4.11")
find_package(PCL REQUIRED COMPONENTS common filters)

add_library(dep_helper INTERFACE IMPORTED)
target_link_libraries(dep_helper INTERFACE doctest TSKPub::messages spdlog fkYAML)
target_include_directories(dep_helper INTERFACE
  ${src_dir}
  ${base_dir}/include
)


file(GLOB reader_test_srcs CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/*.cc)
file(GLOB_RECURSE reader_srcs CONFIGURE_DEPENDS ${src_dir}/*.cc)
add_executable(${PROJECT_NAME} ${reader_test_srcs} ${reader_srcs})
target_include_directories(${PROJECT_NAME} PRIVATE ${src_dir} ${CMAKE_CURRENT_BINARY_DIR} ${PCL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE dep_helper imu Camera xtsdk::xtsdk ${PCL_LIBRARIES})
configure_file(${CONFIG_DIR}/test/unit.yml.in ${CMAKE_CURRENT_BINARY_DIR}/unit.yml)
target_compile_definitions(${PROJECT_NAME} PRIVATE CONFIG_FILE="${CMAKE_CURRENT_BINARY_DIR}/unit.yml")

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
enable_testing()
doctest_discover_tests(${PROJECT_NAME} TEST_PREFIX "unit.")
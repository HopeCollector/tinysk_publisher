cmake_minimum_required(VERSION 3.15)

project(TSKPubStandalong LANGUAGES CXX)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)
CPMUsePackageLock(../package-lock.cmake)
CPMGetPackage(cppzmq)
CPMGetPackage(fkYAML)
CPMGetPackage(spdlog)
CPMAddPackage(
  GITHUB_REPOSITORY jarro2783/cxxopts
  VERSION 3.0.0
  OPTIONS "CXXOPTS_BUILD_EXAMPLES NO" "CXXOPTS_BUILD_TESTS NO" "CXXOPTS_ENABLE_INSTALL YES"
)

CPMAddPackage(NAME tskpub SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

# ---- Create standalone executable ----
add_executable(${PROJECT_NAME} main.cc)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME} TSKPub::TSKPub cppzmq fkYAML cxxopts spdlog)

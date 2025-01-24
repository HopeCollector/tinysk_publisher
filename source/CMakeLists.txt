add_library(${PROJECT_NAME} tskpub.cc reader/imu.cc)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
target_compile_options(${PROJECT_NAME} PRIVATE -std=c++17 -Wall -Wextra -Wpedantic)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    spdlog fkYAML cppzmq PkgConfig::GStreamer imu messages)
target_include_directories(${PROJECT_NAME} 
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/source
)

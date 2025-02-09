add_library(${PROJECT_NAME} tskpub.cc common.cc 
    reader/imu.cc reader/reader.cc reader/cam.cc reader/status.cc)
target_compile_options(${PROJECT_NAME} PRIVATE -std=c++17 -Wall -Wextra -Wpedantic)
target_link_libraries(${PROJECT_NAME}
    PRIVATE spdlog fkYAML cppzmq imu Camera
    PUBLIC messages)
target_include_directories(${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

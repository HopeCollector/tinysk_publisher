# add the CapnProto files
file(GLOB_RECURSE CAPNP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.capnp")

# generate the C++ files
set(msg_include_dir ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/msg)
file(MAKE_DIRECTORY ${msg_include_dir})
set(CAPNPC_OUTPUT_DIR ${msg_include_dir})
capnp_generate_cpp(msgsrc msgheader ${CAPNP_FILES})

# add the generated files to the library
add_library(messages ${msgsrc})
target_link_libraries(messages PUBLIC CapnProto::capnp)
target_include_directories(messages PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}/msg>
)

# install the `messages` library
packageProject(
    NAME messages
    VERSION ${PROJECT_VERSION}
    NAMESPACE ${PROJECT_NAME}
    BINARY_DIR ${PROJECT_BINARY_DIR}
    INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}
    INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
    DEPENDENCIES "CapnProto 1.1.0"
)
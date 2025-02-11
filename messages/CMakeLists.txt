# add the CapnProto files
file(GLOB CAPNP_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.capnp")

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

# add the CapnProto files
file(GLOB_RECURSE CAPNP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.capnp")

# generate the C++ files
set(msgsrcs)
set(msgheaders)
foreach(CAPNP_FILE ${CAPNP_FILES})
    capnp_generate_cpp(msgsrc msgheader ${CAPNP_FILE})
    list(APPEND msgsrcs ${msgsrc})
    list(APPEND msgheaders ${msgheader})
endforeach()

# add the generated files to the library
add_library(messages)
target_sources(messages PRIVATE ${msgsrcs} ${msgheaders})
target_link_libraries(messages PUBLIC CapnProto::capnp)
target_include_directories(messages PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
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
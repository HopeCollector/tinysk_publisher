# CPM Package Lock
# This file should be committed to version control

# PackageProject.cmake
CPMDeclarePackage(PackageProject.cmake
  VERSION 1.13.0
  GITHUB_REPOSITORY TheLartians/PackageProject.cmake
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)
# spdlog
CPMDeclarePackage(spdlog
  NAME spdlog
  VERSION 1.15.0
  GITHUB_REPOSITORY gabime/spdlog
  OPTIONS
    "SPDLOG_BUILD_EXAMPLE OFF"
    "SPDLOG_BUILD_TESTS OFF"
    "SPDLOG_INSTALL ON"
)
# fkYAML
CPMDeclarePackage(fkYAML
  NAME fkYAML
  VERSION 0.4.1
  GITHUB_REPOSITORY fktn-k/fkYAML
  OPTIONS
    "FK_YAML_INSTALL ON"
)
# cppzmq
CPMDeclarePackage(cppzmq
  VERSION 4.10.0
  GITHUB_REPOSITORY zeromq/cppzmq
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)

cmake_minimum_required(VERSION 3.10)
project(HFHub CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find CURL package
find_package(CURL REQUIRED)

# Define the hfhub static library and link it to CURL
add_library(hfhub STATIC src/huggingface_hub.cpp)

# Ensure CURL is available for consumers
target_include_directories(hfhub PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${CURL_INCLUDE_DIRS}
)
target_link_libraries(hfhub PUBLIC CURL::libcurl)

# Export target
install(TARGETS hfhub
  EXPORT hfhubTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)

# Generate a CMake config file for hfhub
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/hfhubConfigVersion.cmake"
  VERSION 1.0
  COMPATIBILITY AnyNewerVersion
)

install(EXPORT hfhubTargets
  FILE hfhubTargets.cmake
  NAMESPACE hfhub::
  DESTINATION lib/cmake/hfhub
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/hfhubConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/hfhubConfig.cmake"
  INSTALL_DESTINATION lib/cmake/hfhub
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/hfhubConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/hfhubConfigVersion.cmake"
  DESTINATION lib/cmake/hfhub
)

add_executable(hfhub_demo src/main.cpp)
target_link_libraries(hfhub_demo hfhub)
install(TARGETS hfhub_demo DESTINATION bin)
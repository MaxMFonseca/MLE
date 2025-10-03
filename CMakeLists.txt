cmake_minimum_required(VERSION 3.16) # Require at least CMake 3.16

# Options to enable/disable features
option(ENABLE_DOXYGEN "Build Doxygen documentation" OFF)
option(BUILD_TOOLS "Build the internal tools" OFF)

project(MLE) # Define the project

# Recursively collect all files in src directory (sources and headers)
file(GLOB_RECURSE MLE_FILES "src/**")

# Filter out files with ".old" in their path
set(FILTERED_FILES "")
foreach(file ${MLE_FILES})
  string(REPLACE "\\" "/" file_path ${file})
  if(NOT file_path MATCHES ".old")
    list(APPEND FILTERED_FILES ${file})
  endif()
endforeach()
set(MLE_FILES ${FILTERED_FILES})

# Create a static library target with the filtered sources and headers
add_library(MLE STATIC ${MLE_FILES})
target_include_directories(MLE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Require C++23 and set common compile options
target_compile_features(MLE PUBLIC cxx_std_23)
target_compile_options(MLE PUBLIC -Wall -Wextra -Wpedantic -Werror -fno-rtti)
target_compile_options(MLE PRIVATE -fno-exceptions)

# Optionally enable Doxygen documentation
if (ENABLE_DOXYGEN)
  find_package(Doxygen REQUIRED)
endif()

# Optionally set log level macros if provided
if (MLE_MAX_LOG_LEVEL)
  target_compile_definitions(MLE PUBLIC MLE_MAX_LOG_LEVEL=${MLE_MAX_LOG_LEVEL})
endif()
if (MLE_DEFAULT_LOG_LEVEL_STDOUT)
  target_compile_definitions(MLE PUBLIC MLE_DEFAULT_LOG_LEVEL_STDOUT=${MLE_DEFAULT_LOG_LEVEL_STDOUT})
endif()

# Add external dependencies
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external)

# Add global compile definitions for third-party libraries and project settings
add_compile_definitions(MLE_APPLICATION)

# Optionally include tests if enabled
if (BUILD_TESTS)
  message(STATUS "Including MLE tests...")
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
endif()

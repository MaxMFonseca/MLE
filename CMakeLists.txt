cmake_minimum_required(VERSION 3.16)

option(ENABLE_DOXYGEN "Build Doxygen documentation" OFF)

add_compile_definitions(
    VULKAN_HPP_NO_EXCEPTIONS
    SOL_NO_EXCEPTIONS
    SPDLOG_NO_EXCEPTIONS
    GLM_ENABLE_EXPERIMENTAL
)

project(MLE)

file(GLOB_RECURSE MLE_SRC "src/**")
file(GLOB_RECURSE MLE_HEADERS "src/**.h" "src/**.inl")

file(GLOB_RECURSE MLE_SRC "src/*")

set(FILTERED_SRC "")
foreach(file ${MLE_SRC})
    string(REPLACE "\\" "/" file_path ${file})
    if(NOT file_path MATCHES ".old")
        list(APPEND FILTERED_SRC ${file})
    endif()
endforeach()
set(MLE_SRC ${FILTERED_SRC})

set(FILTERED_HEADERS "")
foreach(file ${MLE_HEADERS})
    string(REPLACE "\\" "/" file_path ${file})
    if(NOT file_path MATCHES ".old")
        list(APPEND FILTERED_HEADERS ${file})
    endif()
endforeach()
set(MLE_HEADERS ${FILTERED_HEADERS})

add_library(MLE STATIC ${MLE_SRC} ${MLE_HEADERS})
target_include_directories(MLE PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_compile_features(MLE PUBLIC cxx_std_23)
target_compile_options(MLE PUBLIC -Wall -Wextra -Wpedantic -Werror -fno-rtti)

if (ENABLE_DOXYGEN)
    find_package(Doxygen REQUIRED)
endif()
if (MLE_MAX_LOG_LEVEL)
    target_compile_definitions(MLE PUBLIC MLE_MAX_LOG_LEVEL=${MLE_MAX_LOG_LEVEL})
endif()
if (MLE_DEFAULT_LOG_LEVEL_STDOUT)
    target_compile_definitions(MLE PUBLIC MLE_DEFAULT_LOG_LEVEL_STDOUT=${MLE_DEFAULT_LOG_LEVEL_STDOUT})
endif()

target_compile_options(MLE PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-exceptions>
    $<$<CXX_COMPILER_ID:MSVC>:/EHs- /EHa->
)

add_subdirectory(${CMAKE_SOURCE_DIR}/external)
add_subdirectory(${CMAKE_SOURCE_DIR}/tools)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

get_filename_component(test_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# NOTE: quote the glob to avoid list expansion issues on Windows paths
file(GLOB_RECURSE ${test_name}_src
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/**"
)

add_executable(${test_name} ${${test_name}_src} ${MLE_HEADERS})
target_link_libraries(${test_name}
  PRIVATE
    MLE
    GTest::gtest
    GTest::gtest_main
)

target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_compile_options(${test_name} PRIVATE
  -Wall -Wextra -Wpedantic
)

include(GoogleTest)
gtest_discover_tests(${test_name}
  DISCOVERY_MODE PRE_TEST
  PROPERTIES
    LABELS "unit"
)


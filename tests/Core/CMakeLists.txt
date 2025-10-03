get_filename_component(test_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
file(GLOB_RECURSE ${test_name}_src CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/**)
add_executable(${test_name} ${${test_name}_src} ${MLE_HEADERS})
target_link_libraries(${test_name} MLE)


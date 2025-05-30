get_filename_component(tool_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
file(GLOB_RECURSE ${tool_name}_src CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/**)
add_executable(${tool_name} ${${tool_name}_src} ${MLE_HEADERS})
target_link_libraries(${tool_name} MLE)

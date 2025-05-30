file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

message(STATUS "Including all MLE tools:")
foreach(child ${children})
  if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${child}")
    message(STATUS ". ${child}")
    add_subdirectory(${child})
  endif()
endforeach()

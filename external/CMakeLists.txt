add_subdirectory(spdlog)
target_include_directories(MLE PUBLIC spdlog/include)
target_link_libraries(MLE spdlog)

add_subdirectory(glfw)
target_include_directories(MLE PUBLIC glfw/include)
target_link_libraries(MLE glfw)

add_subdirectory(glm)
target_include_directories(MLE PUBLIC glm)
target_link_libraries(MLE glm)

find_package(Vulkan REQUIRED)
target_link_libraries(MLE vulkan)

target_include_directories(MLE PUBLIC LuaJIT/src)
target_link_libraries(MLE ${CMAKE_SOURCE_DIR}/external/LuaJIT/src/libluajit.a)

set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "Build a SPIRV-Reflect static library" FORCE)
add_subdirectory(SPIRV-Reflect)
target_link_libraries(MLE spirv-reflect-static)

find_package(ICU REQUIRED COMPONENTS uc i18n)
target_link_libraries(MLE ICU::uc ICU::i18n)

target_include_directories(MLE PUBLIC stb)

target_include_directories(MLE PUBLIC sol2/include)

target_include_directories(MLE PUBLIC entt/single_include)


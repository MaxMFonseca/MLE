# spdlog: Fast C++ logging library
add_subdirectory(spdlog)
target_include_directories(MLE PUBLIC spdlog/include)
target_link_libraries(MLE spdlog)
target_compile_definitions(MLE PUBLIC SPDLOG_NO_EXCEPTIONS)

# SDL: Simple DirectMedia Layer for windowing, input, etc.
add_subdirectory(SDL)
target_include_directories(MLE PUBLIC SDL/include)
target_link_libraries(MLE SDL3::SDL3)

# glm: Header-only C++ mathematics library for graphics software
add_subdirectory(glm)
target_include_directories(MLE PUBLIC glm)
target_link_libraries(MLE glm)
target_compile_definitions(MLE PUBLIC
  GLM_ENABLE_EXPERIMENTAL GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS)

# Vulkan: Graphics and compute API
find_package(Vulkan REQUIRED)
target_link_libraries(MLE vulkan)
target_include_directories(MLE PUBLIC VULKAN_HPP_NO_EXCEPTIONS)

# LuaJIT: Just-In-Time Compiler for Lua
target_include_directories(MLE PUBLIC LuaJIT/src)
target_link_libraries(MLE ${CMAKE_CURRENT_SOURCE_DIR}/LuaJIT/src/libluajit.a)

# sol2: C++ <-> Lua binding library
target_include_directories(MLE SYSTEM PUBLIC sol2/include)
target_compile_definitions(MLE PUBLIC SOL_NO_EXCEPTIONS)

# SPIRV-Reflect: Reflection library for SPIR-V shader bytecode
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "Build a SPIRV-Reflect static library" FORCE)
add_subdirectory(SPIRV-Reflect)
target_link_libraries(MLE spirv-reflect-static)

# ICU: Unicode and globalization support
find_package(ICU REQUIRED COMPONENTS uc i18n)
target_link_libraries(MLE ICU::uc ICU::i18n)

# OpenAL: Cross-platform 3D audio API
find_package(OpenAL REQUIRED)
target_link_libraries(MLE ${OPENAL_LIBRARY})

# stb: Single-file public domain libraries for images, fonts, etc.
target_include_directories(MLE PUBLIC stb)

# entt: Fast and reliable entity-component system (ECS)
target_include_directories(MLE PUBLIC entt/single_include)
target_compile_definitions(MLE PUBLIC ENTT_NOEXCEPTION)

# utfcpp: UTF-8 handling library
target_include_directories(MLE SYSTEM PUBLIC utfcpp/source)

# dr_libs: Single-file audio decoding libraries
target_include_directories(MLE PUBLIC dr_libs)

# tinygltf: Header-only glTF 2.0 loader and serializer
target_include_directories(MLE PUBLIC tinygltf)

# json: Header-only JSON library for C++
target_include_directories(MLE PUBLIC json)

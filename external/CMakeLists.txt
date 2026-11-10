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
target_link_libraries(MLE Vulkan::Vulkan)
find_path(VMA_INCLUDE_DIR vk_mem_alloc.h
  HINTS ${Vulkan_INCLUDE_DIR}/vma
  REQUIRED
)
target_include_directories(MLE SYSTEM PUBLIC ${VMA_INCLUDE_DIR})

# LuaJIT: Just-In-Time Compiler for Lua
target_include_directories(MLE PUBLIC LuaJIT/src)
if(WIN32)
  set(LUAJIT_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/LuaJIT/src/lua51.lib)
  set(LUAJIT_RUNTIME ${CMAKE_CURRENT_SOURCE_DIR}/LuaJIT/src/lua51.dll)
  if(NOT EXISTS ${LUAJIT_LIBRARY} OR NOT EXISTS ${LUAJIT_RUNTIME})
    message(FATAL_ERROR
      "LuaJIT Windows artifacts are missing. Run scripts/windows.ps1 -Action Setup first.")
  endif()
  add_library(LuaJIT::LuaJIT SHARED IMPORTED GLOBAL)
  set_target_properties(LuaJIT::LuaJIT PROPERTIES
    IMPORTED_IMPLIB ${LUAJIT_LIBRARY}
    IMPORTED_LOCATION ${LUAJIT_RUNTIME}
  )
else()
  add_library(LuaJIT::LuaJIT STATIC IMPORTED GLOBAL)
  set_target_properties(LuaJIT::LuaJIT PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/LuaJIT/src/libluajit.a
  )
endif()
target_link_libraries(MLE LuaJIT::LuaJIT)

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
target_link_libraries(MLE OpenAL::OpenAL)

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

# stdc++exp: C++ experimental library, for stacktrace
# TODO: remove on release builds
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(MLE stdc++exp)
endif()

# ceral: C++11 serialization library
target_include_directories(MLE PUBLIC ceral/include)
target_link_libraries(MLE ${CEREAL_THREAD_LIBS})

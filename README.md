# MLE (Max Lua Engine)

MLE is a custom Vulkan-based voxel path-traced rendering engine purpose-built for large-scale open-world games. It combines high-performance C++ for core systems with Lua for user-modifiable game logic, providing a flexible and efficient development environment.

The engine renders fully procedurally generated terrain, dynamic entities, and complex lighting interactions using a real-time voxel path tracer. It's designed for infinite, modifiable worlds with rich environmental interaction and minimal dependencies.

This doc is very incomplete and will be updated as the project evolves. For now, it serves as a basic overview of the MLE engine and its features.

## Features

- **Vulkan Path Tracing Core**

  - Real-time voxel-based path tracing
  - Block-based global illumination and colored light propagation

- **Voxel World System**

  - Infinite procedurally generated worlds
  - Efficient memory layout with sparse octree entity representation
  - Procedural terrain generation using random noise and fractal algorithms

- **Entity & Chunk Management**

  - Static entities aligned to the voxel grid
  - Sparse octree structure for movable and large-scale entities
  - Dynamic chunk loading and streaming for open-world scalability

- **Modding with Lua**

  - Lua scripting for gameplay and logic
  - Safe bindings and helper functions for geometry, transforms, and asset references

- **UI & Tools**

  - Custom in-engine UI system with post-processing support
  - Toolchain for asset generation, visualization, and debugging

- **Engine Internals**

  - Zero-exception C++23 codebase with `Expected<T>` and `Result`-based error handling
  - Explicit ownership model with `<T>Hnd` (owning) and `<T>Ref` (non-owning)
  - Utility macros for assert, unreachable paths, and safe casting (`as<T>`, `asPtr<T>`)

- Logging and Debugging

  - Spdlog based logging system
  - Many used macros for logging, assertions, and debugging

- Tools build for the engine using the engine

  - These can be found in the `tools` directory and are built with the engine.
  - Currently developing the MLECubes tool that will help create and visualize voxel-based assets.

## Building

> **Requirements:**
>
> - C++23 compiler (Clang or GCC)
> - CMake 3.18+
> - Vulkan SDK
> - Git submodules (`git submodule update --init --recursive`)

We use a helper script to set up the environment and build the project. It detects the platform and configures everything automatically:

```bash
. ./scripts/envsetup.sh
mle_setup -> Fetches all submodules and set up the environment
mle_config -> Config cmake and tooling
mle_build -> Build the engine and tools
```

## Running a Tool

Use the helper script to run any internal tool:

```bash
./scripts/run_tool.sh -n <tool_name> -t Release -- "--arg1 value1 --arg2"
```

Arguments after `--` are passed directly to the tool. The script handles environment configuration and ensures the correct build is used.

## Coding Conventions

- No exceptions: all errors use `Expected<T>` and `Result`
- Ownership is explicit:
  - `<T>Hnd` = owning (`std::unique_ptr<T>`)
  - `<T>Ref` = non-owning, must outlive the receiver
- Safe casting helpers:
  - `as<T>` replaces `static_cast<T>`
  - `asPtr<T>` replaces `reinterpret_cast<T*>`
- All public API elements must be documented using Doxygen-style comments
- Inline and field comments are preferred for simplicity and readability

## License

This project is private and not licensed for external distribution at this time.

## MLE

MLE is a custom Vulkan-based rendering engine built for large-scale open-world games. It combines high-performance C++ for core systems with Lua for user-modifiable game logic, providing a flexible and efficient development environment.

The engine is designed to render fully procedural terrain, dynamic entities, and complex lighting interactions. It targets infinite, modifiable worlds with rich environmental interaction and minimal external dependencies.

This document is still incomplete and may contain outdated or inaccurate information. It will evolve alongside the project. For now, it serves as a high-level overview of the MLE engine and its current direction.

## Planned Features

* **Vulkan Core**

  * Real-time rendering systems

* **Entity and Chunk Management**

  * Static entities aligned to the voxel grid
  * Sparse octree structures for movable and large-scale entities
  * Dynamic chunk loading and streaming for open-world scalability

* **Modding with Lua**

  * Lua scripting for gameplay and logic
  * Safe bindings and helper functions for geometry, transforms, and asset references

* **UI and Tools**

  * Custom in-engine static UI system with post-processing support
  * Tooling for asset generation, visualization, and debugging

* **Engine Internals**

  * Zero-exception C++23 codebase using `Expected<T>` and `Result`-based error handling
  * Explicit ownership model with `<T>Hnd` for owning references and `<T>Ref` for non-owning references
  * Utility macros for assertions, unreachable paths, and safe casting (`as<T>`, `asPtr<T>`)

* **Logging and Debugging**

  * `spdlog`-based logging system
  * Extensive macro support for logging, assertions, and debugging

* **Engine Tools**

  * Internal tools built using the engine itself
  * These tools are located in the `tools` directory
  * The currently in-development `MLECubes` tool helps create and visualize voxel-based assets

## Building

> **Requirements**
>
> * Clang 17+
> * CMake 3.18+
> * Vulkan SDK 1.3+

A helper script is provided to automate most of the setup process:

```bash
source ./scripts/envsetup.sh
mle_setup   # Fetches all submodules and sets up the environment
mle_config  # Configures CMake and tooling
mle_build   # Builds the engine and tools
```

## Running a Tool

Use the helper script to run any internal tool:

```bash
./scripts/run_tool.sh -n <tool_name> -t Release -- "--arg1 value1 --arg2"
```

Arguments after `--` are passed directly to the tool. The script handles environment configuration and ensures the correct build is used.

## Coding Conventions

* Exceptions are disabled; all errors use `Expected<T>` and `Result`
* Ownership is explicit:

  * `<T>Hnd` = owning (`std::unique_ptr<T>`)
  * `<T>Ref` = non-owning and must outlive the receiver
* Safe casting helpers:

  * `as<T>` replaces `static_cast<T>`
  * `asPtr<T>` replaces `reinterpret_cast<T*>`
* Public API elements should be documented with Doxygen-style comments
* Inline and field comments are preferred where they improve clarity without adding noise

## License

This project is private and is not currently licensed for external distribution.

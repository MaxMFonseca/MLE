# MLE

MLE is a custom Vulkan based rendering engine purpose-built for large-scale open-world games. It combines high-performance C++ for core systems with Lua for user-modifiable game logic, providing a flexible and efficient development environment.

The engine renders fully procedurally generated terrain, dynamic entities, and complex lighting interactions. It's designed for infinite, modifiable worlds with rich environmental interaction and minimal dependencies.

This doc is very incomplete, and possibly wrong. It will be updated as the project evolves. For now, it serves as a basic overview of the MLE engine and its features.

## Planed Features

- **Vulkan Core**

  - Real-time rendering systems

* **Modding with Lua**

  * Lua scripting for gameplay and logic
  * Safe bindings and helper functions for geometry, transforms, and asset references

* **UI and Tools**

  * Custom in-engine static UI system with post-processing support
  * Tooling for asset generation, visualization, and debugging

* **Engine Internals**

  - Custom in-engine static UI system with post-processing support
  - Toolchain for asset generation, visualization, and debugging

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
> - Clang v17+
> - CMake 3.18+
> - Vulkan SDK 1.3+

Then there is a helper script that automates the rest or the process:

```bash
source ./scripts/envsetup.sh
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

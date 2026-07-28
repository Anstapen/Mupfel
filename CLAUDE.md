# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Mupfel is a C++20 2D game engine with a focus on simplicity. Entity behavior is established using a lightweight and performant ECS. Every system apart from the Rendering is done on the GPU.

Windowing and rendering are built on Vulkan via the vendored `Ping` library (see below): `Window` wraps
GLFW, and `Renderer` drives `Ping`'s Vulkan device/swapchain/pipeline directly.

The repo follows a `Core`/`App` split (originally generated from a C++ Premake starter template):
`Core` builds as a static library containing all reusable engine code; `App` builds the executable,
links `Core`, and contains game/editor-specific layers.

## Build

Build files are generated with Premake5 (vendored binaries, not a system install). The build is split
into three tiers, each with its own `Build-*.lua` (see `BUILD.md` for the full dependency graph):
`Vendor/Build-Vendor.lua` (third-party libraries built from vendored source: spdlog, imgui, Logger,
Ping), `Core/Build-Core.lua` (the `Core` engine static lib), and `App/Build-App.lua` (the `App`
executable). `Build.lua` at the repo root wires these together and defines
`ApplyDefaultProjectSettings()`, the shared per-project toolchain/output configuration every project
calls into. `Dependencies.lua` is the single source of truth for where each vendored dependency's
source lives and how to fetch it (`Deps` table, `DepPath()` helper, `fetch_dependency()`).

- Windows: run `Scripts/Setup-Windows.bat` (generates a VS2026 solution at the repo root via
  `premake5.exe --file=Build.lua vs2026`), then build with MSBuild or open the generated `.slnx` in
  Visual Studio. This requires the vendored `premake5.exe` (beta8+, under
  `Vendor/Binaries/Premake/Windows/`) — older Premake builds don't have the `vs2026` action.
- Linux: run `Scripts/Setup-Linux.sh` (generates gmake2 project files via
  `premake5 --cc=clang --file=Build.lua gmake2`), then `make`.

Both scripts must be run from inside `Scripts/` (they `pushd ..` to reach the repo root first).

On first run, `Dependencies.lua` (included by `Build.lua`) downloads and unzips missing third-party
sources into `Vendor/Sources/`: glm, nlohmann/json, the `vulkan_starter` repo (for `Ping`), spdlog,
ImGui (pinned docking-branch commit), stb_image, and (Windows only) a prebuilt GLFW release. This
requires network access; if `Vendor/Sources/` already contains a dependency's directory, it's skipped.

Building requires the **Vulkan SDK** installed with the `VULKAN_SDK` environment variable set —
`Build.lua` aborts immediately if it isn't found, since `Vendor/Build-Vendor.lua` needs its headers/libs
to build `Ping`, which `Core` depends on directly (see below).

There are three configurations: `Debug`, `Release`, `Dist` (see `Core/Build-Core.lua` /
`App/Build-App.lua` for the exact defines/runtime settings per configuration). Output binaries land in
`Binaries/<system>-<arch>/<config>/<project>/`.

There is no test suite in this repository.

## Architecture

### Application / Layer lifecycle

`Application` (`Core/Source/Core/Application.h`) is a singleton owning the window, event system, input
manager, ECS registry, physics simulation, thread pool, and a permanent `DebugLayer`. Game/editor code
subclasses `Layer` (`OnInit`/`OnEvent`/`OnUpdate`/`OnRender`) and is registered via
`Application::PushLayer<T>()` before calling `Application::Run()` (see `App/Source/App.cpp`).

`Application::Run()` drives the frame loop in a fixed order: poll window-level input events → update
layers → step physics (`PhysicsSimulation::Update`, which sub-steps `MovementSystem` and
`CollisionSystem`) → render engine layer (`Renderer::RenderNextFrame()`) → render app layers → render
`DebugLayer` if debug mode is toggled → swap the event system's buffers → update `InputManager`. Debug
mode is toggled at runtime via a `UserInput::TOGGLE_DEBUG_MODE` input event, not a build flag.

`Application`/`Registry`/`EventSystem`/etc. are accessed through static `Application::GetCurrent*()`
accessors rather than passed around explicitly.

### ECS (`Core/Source/ECS`)

Sparse-set ECS. `Entity` is just a 4-byte index (`Core/Source/ECS/Entity.h`); `Registry` owns
per-component-type storage plus a signature (`std::bitset<64>`) per entity used for archetype-style
queries.

Component storage is host-memory only: `CPUComponentArray<T>` — a plain `std::vector`-backed sparse set
(parallel `sparse`/`dense`/`components` arrays, O(1) swap-remove). `Registry::GetComponentArray<T>()`
lazily allocates one `CPUComponentArray<T>` per component type on first use.

`Registry::view<Components...>()` (`ECS/View.h`) iterates entities matching a component signature.
`Registry::ParallelForEach<Components...>(fn, changed_entities)` splits the dense array of the first
component type across the engine thread pool (`Application::GetCurrentThreadPool()`) and calls `fn` per
matching entity; entities for which `fn` returns `true` are collected into `changed_entities`. This is
the standard way to write a CPU-side system that touches many entities (see `MovementSystem`).

Adding/removing a component fires `ComponentAddedEvent`/`ComponentRemovedEvent` through the
`EventSystem` — systems that need to react to structural ECS changes should listen for these rather
than polling.

### Event system (`Core/Source/Core/EventSystem.h`, `EventBuffer.h`)

Double-buffered: events added via `AddEvent<T>()` this frame are only visible via `GetEvents<T>()`
starting *next* frame (buffers are swapped once per frame in `Application::Run()`). `AddImmediateEvent<T>()`
additionally invokes any callbacks registered with `RegisterListener<T>()` synchronously, in the same
frame. Event types are not registered up front — each `T` derived from `Event` gets a lazily-assigned
type ID the first time it's used, and buffers are created on demand.

### Rendering



### Debug layer



### Ping (Vulkan wrapper, vendored)

`Ping` (`Vendor/Build-Vendor.lua`) is a Vulkan rendering wrapper library pulled in from the `main`
branch of [Anstapen/vulkan_starter](https://github.com/Anstapen/vulkan_starter) (vendored to
`Vendor/Sources/vulkan_starter-main/Ping/`) and built as its own static-lib project that `Core` links
against — its `Source/Ping` and `Source/Vulkan` headers are on `Core`'s include path
(`#include "Ping/Ping.h"` etc.). It brings its own transitive dependencies, also vendored and built as
separate static-lib projects in `Vendor/Build-Vendor.lua`: spdlog (logging), Dear ImGui (pinned
docking-branch commit, GLFW backend only), stb_image, and a prebuilt GLFW 3.4 Windows binary. `Core`'s
`links{}` pulls in `Ping`, `spdlog`, `imgui`, `glfw3`, and `vulkan` so the final `App` executable
resolves them transitively. `Ping` is the sole windowing/rendering dependency, used directly by
`Window` (GLFW handle), `Application` (`Ping::Init`/`Ping::Device`), and `Renderer` (swapchain,
pipeline, buffers, and the ImGui-based `Ping::Gui`).

Note: spdlog's bundled `fmt` headers require MSVC's `/utf-8` flag, added workspace-wide in `Build.lua`
for this reason.

## Third-party dependencies

Vendored under `Vendor/Sources/` (downloaded on first Premake run, gitignored): glm (math), nlohmann/json
(used by `FS/EntityFileManager` for entity serialization), Box2D v3.1.1 (collision detection/resolution,
linked by `Core`), and Ping's dependency chain (`vulkan_starter`, spdlog, ImGui, stb_image, GLFW)
described above. Premake binaries themselves are checked into `Vendor/Binaries/`.

Box2D 3.x is a **C** library (not C++ like the 2.x line), so its project in `Vendor/Build-Vendor.lua` is
the only one overriding `ApplyDefaultProjectSettings()`'s language/dialect (`language "C"`,
`cdialect "C17"`) — see BUILD.md's "Box2D — the one project that isn't C++" for the MSVC flag and
determinism caveats. Its public headers are `extern "C"`, so C++ code just does `#include <box2d/box2d.h>`.

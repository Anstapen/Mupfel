# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Mupfel is a C++20 2D game engine with a focus on simplicity. Entity behavior is established using a lightweight and performant ECS. Every system apart from the Rendering is done on the GPU.

Windowing and rendering are built on Vulkan via the vendored `Ping` library (see below): `Window` wraps
GLFW, and `Renderer` drives `Ping`'s Vulkan device/swapchain/pipeline directly.

The repo follows a `Core`/`App` split (originally generated from a C++ Premake starter template):
`Core` builds as a static library containing all reusable engine code; `App` builds the executable,
links `Core`, and contains game/editor-specific layers. `Core`'s headers are further split into a
published surface (`Core/Include`) and engine internals (`Core/Source`) — see "Public vs. private
headers" below, since that split decides where a new header belongs.

## Build

Build files are generated with Premake5 (vendored binaries, not a system install). Each tier has its
own `Build-*.lua` (see `BUILD.md` for the full dependency graph): `Vendor/Build-Vendor.lua`
(third-party libraries built from vendored source: spdlog, imgui, Logger, Ping),
`Core/Build-Core.lua` (the `Core` engine static lib), `App/Build-App.lua` (the `App` executable, and
the workspace's `startproject`), plus `Tests/Build-Tests.lua` and
`Benchmarks/Build-Benchmarks.lua`, which both link `Core`. `Build.lua` at the repo root wires these
together and defines
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

`Tests` is a Catch2 unit-test executable (`Tests/Source`) and `Benchmarks` a nanobench microbenchmark
executable (`Benchmarks/Source`); both link `Core` and are built by the same solution. Run them
directly from their output directories — there is no CLI test runner or lint step wired into the
build.

### Public vs. private headers

`Core`'s headers live under two roots, and which one a header is in is a real access-control
boundary, not a convention:

- `Core/Include` — the published surface (33 headers). `Mupfel.h` is the umbrella entry point that
  pulls in everything an application needs; the individual headers stay includable on their own for
  translation units that want a narrower, cheaper-to-compile surface.
- `Core/Source` — engine internals plus every `.cpp`. Currently `Core/Debug/DebugLayer.h`,
  `Core/DefaultScene.h`, `Core/GUID.h`, `Core/ResourceManager.h`, `Physics/CollisionSystem.h`,
  `Physics/MovementSystem.h`, `Physics/PhysicsSimulation.h`, `Renderer/AnimationSystem.h`,
  `Renderer/DebugRenderer.h`, `Renderer/ECSRenderer.h`, `Renderer/IMRenderer.h`, `Renderer/Quad.h`,
  `Renderer/Renderer.h`, `Renderer/SubRenderer.h`.

Premake has no CMake-style `PUBLIC`/`PRIVATE` include model, so the enforcement *is* the directory
split: `App` puts only `Core/Include` on its include path and cannot reach `Core/Source` at all.
`Tests` and `Benchmarks` are deliberately white-box and get both roots (they exercise
`ResourceManager.h`, which is private).

Two consequences when adding code:

- A new header reachable from an existing public header must itself be public, or `App` won't
  compile. Keeping something private means keeping it out of the public headers' include closure.
- `Core`'s own `includedirs` lists each public subdirectory (`Include/Core`, `Include/ECS`, …)
  alongside the two roots, because existing engine code includes siblings by bare name
  (`#include "SubRenderer.h"`), which stops resolving once the sibling lives in the other root. All
  public header basenames are unique, so the flattened search is unambiguous. Path-qualifying those
  includes (`"Renderer/SubRenderer.h"`) would let the subdirectory entries be dropped.

### Vendor visibility

`App/Build-App.lua`'s `includedirs` is down to `"Source"`, `Core/Include`, spdlog and nlohmann json.
Ping, the Vulkan SDK, GLFW, ImGui, Box2D and glm are **fully hidden** behind `Core`'s public headers
and are not reachable from application code. This was verified by compiling every public header
against exactly those paths and nothing else; re-run that check after changing a public header.

spdlog and nlohmann json are *deliberately published*, not leaked — they are considered useful to the
application author:

- `Core/Include/Core/Logger.h` is Mupfel's own logging header (`Logger::SafeLoggerPtr` =
  `std::shared_ptr<spdlog::logger>`, plus the shared console/file sinks). It replaced the
  `Logger/Logger.h` that used to come from `Ping`, which is what previously forced Ping's include
  path onto `App`.
- `FS/EntityFileManager.h` names `nlohmann::json` in its public `ComponentLoader` signature.

Both entries are load-bearing: removing either breaks a public header. Don't "clean them up".

The techniques that got the rest hidden, and the rules that keep them working:

- **Forward declaration + `std::unique_ptr` for leaky members.** `Application` forward-declares
  `Renderer`, `PhysicsSimulation`, `AnimationSystem`, `DebugLayer`, `Ping::Device` etc. and holds
  them by `unique_ptr` instead of by value. A **by-value** member always needs a complete type — the
  compiler needs its size for layout, and `private` doesn't change that. So does `std::optional<T>`,
  which is why `gpu` became `unique_ptr<Ping::Device>` rather than staying an `optional`.
- **`~Application()` must stay out-of-line** (declared in `Application.h`, defined in
  `Application.cpp:61`). `unique_ptr<T>` instantiates its deleter in the destructor, which needs a
  complete `T`. Letting the compiler generate it in the header breaks every construction site with a
  confusing error. Same rule for any move constructor/assignment added later.
- **Inline member functions defeat forward declarations.** An inline getter that dereferences a
  forward-declared member, or an inline constructor that calls into it, forces the include back. Keep
  those bodies in the `.cpp`.
- **`Window.h` / `InputManager.h` forward-declare `GLFWwindow`** (`struct GLFWwindow;`) rather than
  including GLFW. That is why GLFW never appears in `App`'s include path.

One fragile spot to be aware of: `Application` holds `ImageManager` **by value**, and
`Renderer/ImageManager.h` holds `std::vector<Ping::Image>` with `Ping::Image` only forward-declared.
That is legal since C++17 and compiles only because `~Application()` is out-of-line where `Ping::Image`
is complete. It breaks if anything else instantiates an `ImageManager` destructor. Splitting the header
(public `ImageTypes.h` with `ImageHandle`/`ImageSpecification`, private `ImageManager` class) would
remove the hazard — `App` only ever needs those two trivial types, since it goes through
`Application::LoadBasicImage` and friends.

## Architecture

### Application / Layer lifecycle

`Application` (`Core/Include/Core/Application.h`) is a singleton owning the window, event system, input
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

### ECS (`Core/Include/ECS`)

Sparse-set ECS. `Entity` is just a 4-byte index (`Core/Include/ECS/Entity.h`); `Registry` owns
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

### Event system (`Core/Include/Core/EventSystem.h`, `EventBuffer.h`)

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

`Ping` is entirely engine-internal: it is on `Core`'s include path but **not** on `App`'s, so no
`Ping::` type is nameable from application code. `Application.h` only forward-declares `Ping::Device`
to hold it by `unique_ptr` (see "Vendor visibility"). Note that `Core/Include/Core/Logger.h` is
Mupfel's own header and no longer the `Logger/Logger.h` that ships inside `Ping` — that swap is what
freed `App` from Ping's include path, since both live under the same root
(`Vendor/Sources/vulkan_starter-main/Ping/Source/`).

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

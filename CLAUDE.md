# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Mupfel is a C++20 2D game engine with a focus on simplicity. Entity behavior is established using a lightweight and performant ECS. Every system apart from the Rendering is done on the CPU.

Windowing and rendering are built on Vulkan: `Window` wraps GLFW, `Renderer` drives NVIDIA's
[NVRHI](https://github.com/NVIDIA-RTX/NVRHI) (see below), and the Vulkan instance/device/swapchain that
NVRHI does not create come from vk-bootstrap.

The repo follows a `Core`/`App` split:
`Core` builds as a static library containing all reusable engine code; `App` builds the executable,
links `Core`, and contains game/editor-specific layers. `Core`'s headers are further split into a
published surface (`Core/Include`) and engine internals (`Core/Source`) — see "Public vs. private
headers" below, since that split decides where a new header belongs.

## Build

CMake (4.0+), driven entirely through `CMakePresets.json` — there are no setup scripts. `BUILD.md` has
the full dependency graph and the rationale behind everything here.

```
cmake --preset windows            # Visual Studio 2026 solution in Build/windows
cmake --preset windows-ninja      # Ninja Multi-Config; use this one for CLion
cmake --preset linux              # Ninja Multi-Config

cmake --build --preset windows-debug        # or windows-release
ctest --preset windows-debug
```

Each tier has its own `CMakeLists.txt` (`Vendor/`, `Core/`, `App/`, `Tests/`); the root one wires them
together. Shared logic lives in `cmake/`: `MupfelSettings.cmake` (`mupfel_apply_default_settings()`,
`mupfel_apply_strict_warnings()`), `Dependencies.cmake` (where each vendored source lives and how to
fetch it), `Modules.cmake` (the `MUPFEL_MODULES` option — `-D MUPFEL_MODULES=core` builds the engine
alone), and `VulkanSDK.cmake` / `GLFW.cmake`.

**`CC`/`CXX` naming a non-MSVC compiler will hijack the Windows build** and fail deep inside the MSVC
standard library. The `windows-ninja` preset pins `cl` and clears both for that reason. On Linux, GCC 14+
is required (`std::ranges::to`) plus `libglfw3-dev`.

On first configure, missing third-party sources are downloaded into `Vendor/Sources/` (glm, nlohmann,
NVRHI, vk-bootstrap, spdlog, ImGui, stb_image, Catch2, and on Windows a prebuilt GLFW). Upstream's own
`CMakeLists.txt` files are deliberately never used — every vendored library is rebuilt as one of our own
targets.

Building requires the **Vulkan SDK** with `VULKAN_SDK` set. `cmake/VulkanSDK.cmake` aborts if it is
missing, and also aborts if `VK_HEADER_VERSION` is below **357** (SDK 1.4.357): NVRHI hard-`#error`s
below 318 and vk-bootstrap needs headers matching its pinned tag, so without the check both fail deep
inside a compile instead of at configure time. That floor and the `vk_bootstrap` pin move together.

Two configurations, `Debug` and `Release`. Release is optimized *with* symbols, which needs `/DEBUG` on
the linker (CMake adds it only for `Debug`/`RelWithDebInfo`), and debug info is `Embedded` (`/Z7`) rather
than `/Zi` — see BUILD.md, both are load-bearing. Output binaries land in
`Binaries/<system>-<arch>/<config>/<project>/`.

`Tests` is a Catch2 unit-test executable (`Tests/Source`); it links `Core` and is registered with CTest.
There is no lint step wired into the build.

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

The enforcement is twofold. The directory split holds — `App` cannot reach `Core/Source` at all — and
`Core`'s link interface states it: `target_include_directories(Core PUBLIC Include ...)` publishes only
the header root, everything else is `PRIVATE`. `Tests` is deliberately white-box and adds `Core/Source`
itself (it exercises `ResourceManager.h`, which is private).

Three consequences when adding code:

- A new header reachable from an existing public header must itself be public, or `App` won't
  compile. Keeping something private means keeping it out of the public headers' include closure.
- `Core` lists each public subdirectory (`Include/Core`, `Include/ECS`, …) as **PRIVATE** alongside the
  two roots, because engine code includes siblings by bare name (`#include "SubRenderer.h"`), which
  stops resolving once the sibling lives in the other root. All public header basenames are unique, so
  the flattened search is unambiguous. They are PRIVATE so `App` can't resolve a bare
  `"SubRenderer.h"` of its own; path-qualifying those includes would let the entries be dropped.
- A vendored library that should stay hidden goes in the `PRIVATE` half of
  `target_link_libraries(Core ...)`. `Core` is a static library, so `PRIVATE` still propagates on the
  *link line* while include directories and defines do not — which is what hides NVRHI and friends from
  `App` by construction rather than by a hand-maintained list.

### Vendor visibility

`App/CMakeLists.txt` names `"Source"` and `Core`, nothing else. `Core/Include`, spdlog and nlohmann json
arrive as `Core`'s PUBLIC usage requirements; NVRHI, vk-bootstrap, the Vulkan SDK, GLFW, ImGui, Box2D and
glm are **fully hidden** because `Core` links them `PRIVATE`. This is enforced by the build graph rather
than by keeping a list correct.

spdlog and nlohmann json are *deliberately published*, not leaked — they are considered useful to the
application author:

- `Core/Include/Core/Logger.h` is Mupfel's own logging header (`Logger::SafeLoggerPtr` =
  `std::shared_ptr<spdlog::logger>`, plus the shared console/file sinks). It replaced the
  `Logger/Logger.h` that used to come from `Ping`, which is what previously forced Ping's include
  path onto `App`. It is unaffected by the NVRHI migration.
- `FS/EntityFileManager.h` names `nlohmann::json` in its public `ComponentLoader` signature.

Both entries are load-bearing: removing either breaks a public header. Don't "clean them up".

The techniques that got the rest hidden, and the rules that keep them working:

- **Forward declaration + `std::unique_ptr` for leaky members.** `Application` forward-declares
  `Renderer`, `PhysicsSimulation`, `AnimationSystem`, `DebugLayer`, the GPU device etc. and holds
  them by `unique_ptr` instead of by value. A **by-value** member always needs a complete type — the
  compiler needs its size for layout, and `private` doesn't change that. So does `std::optional<T>`,
  which is why `gpu` became a `unique_ptr` rather than staying an `optional`.
- **`~Application()` must stay out-of-line** (declared in `Application.h`, defined in
  `Application.cpp:61`). `unique_ptr<T>` instantiates its deleter in the destructor, which needs a
  complete `T`. Letting the compiler generate it in the header breaks every construction site with a
  confusing error. Same rule for any move constructor/assignment added later.
- **Inline member functions defeat forward declarations.** An inline getter that dereferences a
  forward-declared member, or an inline constructor that calls into it, forces the include back. Keep
  those bodies in the `.cpp`.
- **`Window.h` / `InputManager.h` forward-declare `GLFWwindow`** (`struct GLFWwindow;`) rather than
  including GLFW. That is why GLFW never appears in `App`'s include path.

One spot the NVRHI migration forces a decision on: `Application` holds `ImageManager` **by value**, and
`Renderer/ImageManager.h` used to hold `std::vector<Ping::Image>` with `Ping::Image` only
forward-declared. That was legal since C++17 and compiled only because `~Application()` is out-of-line.
The equivalent trick is **not** available with NVRHI: `nvrhi::TextureHandle` is
`RefCountPtr<ITexture>`, a class template instantiation that cannot be forward-declared the way an
opaque class could. The header split that was previously optional is therefore now required — public
`ImageTypes.h` carrying `ImageHandle`/`ImageSpecification`, private `ImageManager` class — and it is
a clean split, because `App` only ever needs those two trivial types anyway: it goes through
`Application::LoadBasicImage` and friends.

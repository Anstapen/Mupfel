# Build system structure

This document describes how Mupfel's Premake build is organized: which libraries exist, what each one
depends on, and which files are responsible for defining them. It reflects the restructuring done to
give the build a clear three-tier separation — **Vendor** (third-party code) / **Engine** (`Core`) /
**Application** (`App`) — instead of vendor projects being defined inline inside `Core`'s build script.

## Directory layout

```
Build.lua                  Workspace root: declares the workspace, shared per-project settings
                            (ApplyDefaultProjectSettings), the strict-warning opt-in for our own
                            code (ApplyStrictWarnings), and the Vendor/Engine/App group structure.
Dependencies.lua           Single source of truth for third-party dependencies: where each one's
                            source lives (Deps table) and how to fetch it (DepPath, fetch_dependency).

Vendor/
  Build-Vendor.lua         Builds vendored third-party static libs: spdlog, imgui, nvrhi, nvrhi_vk,
                            vk-bootstrap, box2d, catch2.
  Sources/                 Fetched/vendored source trees (gitignored, populated by Dependencies.lua).
  Binaries/Premake/        Vendored premake5 executables (checked into git).

Core/
  Build-Core.lua           Builds the "Core" engine static lib from Core/Source.
  Source/                  Mupfel's own engine code (ECS, physics, renderer, event system, ...).

App/
  Build-App.lua            Builds the "App" executable from App/Source (+ App/Shaders).
  Source/                  Game/editor-specific layers.

Tests/
  Build-Tests.lua          Builds the "Tests" executable (unit tests) from Tests/Source.
  Source/                  Catch2-based unit tests (links Core + catch2; see its README).
```

Each tier's build script only exists inside that tier's own directory, and only defines projects that
belong to that tier — `Vendor/Build-Vendor.lua` never reaches into `Core/` or `App/`, and vice versa.

## Dependency graph

```
                         ┌─────────────┐
                         │ Vulkan SDK  │  (system dependency, VULKAN_SDK env var — not vendored.
                         └──────┬──────┘   Build.lua asserts VK_HEADER_VERSION >= 357; see below)
                                │ headers
     ┌────────────┐             │             ┌────────────┐
     │   glfw3    │             │             │   spdlog   │  (no internal deps)
     │ (prebuilt, │             │             └────────────┘
     │  Windows)  │             ├──────────────────┐
     └─────┬──────┘             │                  │ headers
           │ headers            │                  ▼
           │              ┌─────▼──────┐    ┌──────────────┐
           └─────────────►│   imgui    │    │ vk-bootstrap │  instance/device/queue/swapchain
                          └────────────┘    └──────┬───────┘  creation — the part NVRHI does not do
                                                   │
                          ┌────────────┐           │
                          │   nvrhi    │  NVRHI core: common utilities + validation layer
                          └─────┬──────┘
                                │ link
                          ┌─────▼──────┐
                          │  nvrhi_vk  │  NVRHI's Vulkan backend (needs the SDK's headers)
                          └─────┬──────┘
                                │ link       │ link
     ┌────────────┐        ┌────▼────────────▼───────┐
     │   box2d    ├───────►│          Core           │  Mupfel's engine (Core/Include + Core/Source)
     │  (C17, no  │  link  │  links: nvrhi_vk, nvrhi, │  headers: nlohmann, glm, nvrhi,
     │  int. deps)│        │  vk-bootstrap, spdlog,   │           vk-bootstrap, stb, vulkan,
     └────────────┘        │  imgui, glfw3, vulkan,   │           glfw, spdlog, imgui, box2d
                           │  box2d (+ dl on Linux)   │
                           └────────────┬─────────────┘
                                        │ link
                           ┌────────────▼─────────────┐
                           │           App            │  Game/editor (App/Source)
                           │  links: Core             │  headers: spdlog, nlohmann
                           └──────────────────────────┘
```

`Tests` hangs off `Core` the same way `App` does — a `ConsoleApp` linking the engine, kept out of the
diagram above along with its catch2 framework rather than folded into it.

Header-only dependencies (no build project, just `includedirs`): **nlohmann/json**, **glm**,
**stb_image**. `stb_image` used to be compiled inside `Ping`; with `Ping` gone, `Core` owns the single
translation unit that defines `STB_IMAGE_IMPLEMENTATION`. `nlohmann` is used by both `Core` (entity
serialization) and `App`; `glm` (math) is used by `Core`'s renderer. **catch2** is *not* header-only — see
"Catch2" below for why it gets a project of its own despite shipping as two files.

**Who links what:**

| Project        | Links against                                      | Also sees headers of (no link) |
|----------------|----------------------------------------------------|---------------------------------|
| `spdlog`       | —                                                  | —                                |
| `imgui`        | —                                                  | glfw                             |
| `nvrhi`        | —                                                  | —                                |
| `nvrhi_vk`     | — (symbols resolved from `nvrhi` at final link)    | nvrhi, vulkan                    |
| `vk-bootstrap` | — (dlopens the Vulkan loader at runtime)           | vulkan                           |
| `box2d`        | —                                                  | —                                |
| `catch2`       | —                                                  | —                                |
| `Core`         | nvrhi_vk, nvrhi, vk-bootstrap, spdlog, imgui, glfw3, vulkan, box2d, `dl` (Linux) | nlohmann, glm, stb |
| `App`          | Core                                               | spdlog, nlohmann                 |
| `Tests`        | Core, catch2                                       | + Core's header set (anything reaching Application.h) |

This table is the direct answer to "who includes which headers" — it's now also mechanically
enforced: each project's `Build-*.lua` file only calls `includedirs`/`links` for what it actually
uses, resolved via `DepPath(name, subpath)` (see below) rather than ad hoc relative path strings.

## How dependency paths are resolved

Previously, each vendored dependency's directory name and version were duplicated as literal strings
in both `Dependencies.lua` (for downloading) and `Core/Ping.lua` (for `includedirs`/`files`). A version
bump meant updating two files and could silently desync (this had already happened for `glm`: it was
never added to `Dependencies.lua`'s downloader at all, so a fresh clone would fail to build `App`
until someone manually placed it in `Vendor/Sources/`).

Now `Dependencies.lua` defines one `Deps` table as the single source of truth:

```lua
Deps = {
    spdlog = { relpath = "Vendor/Sources/spdlog-1.17.0", url = "https://.../v1.17.0.zip" },
    ...
}
```

`DepPath(name, subpath)` turns a `Deps` entry into a path anchored to the workspace root via the
`%{wks.location}` Premake token, so it resolves correctly regardless of which project file calls it:

```lua
DepPath("spdlog", "include")  -->  "%{wks.location}/Vendor/Sources/spdlog-1.17.0/include"
```

`fetch_dependency(name)` downloads/extracts a dependency into `Vendor/Sources/` if it isn't already
present, driven entirely by the same `Deps` entry. Which fields the entry sets decide the shape:

| Fields                  | Meaning                                                                      |
|-------------------------|------------------------------------------------------------------------------|
| `url` + `relpath`       | `url` is a zip archive, extracted into `Vendor/Sources/`                     |
| `+ single_file`         | `url` points straight at one loose file (`json.hpp`, `stb_image.h`)          |
| `single_files`          | a list of full file URLs, for deps shipped as a few loose files (`catch2`)   |

`single_files` checks and fetches each file on its own, so an interrupted run resumes instead of
leaving a half-populated directory behind — the directory merely existing would otherwise read as
"present" to the `os.isdir` check and the remaining files would never be downloaded.

`build_externals()` calls `fetch_dependency` for every dependency Mupfel needs, including the
previously-missing `glm`, and only fetches the Windows GLFW binary when actually targeting Windows
(see "Bugs fixed" below).

Adding a new vendored dependency is now one entry in `Deps` plus a project block (if it needs
compiling) in `Vendor/Build-Vendor.lua` — no path strings duplicated elsewhere.

## Shared project settings

Every project (vendor, engine, app) previously repeated ~30 lines of identical filter blocks
(per-configuration defines/runtime/symbols, MSVC character set, output directories). This is now
`ApplyDefaultProjectSettings()`, defined once in `Build.lua` and called as the first line of every
`project` block:

```lua
project "Core"
    kind "StaticLib"
    ApplyDefaultProjectSettings()
    -- project-specific files/includedirs/links follow
```

Only what's genuinely project-specific (`kind`, `files`, `includedirs`, `links`, extra `defines`)
stays in each `Build-*.lua` file.

## Warnings: strict for our code, silent for vendored code

`Core` and `Tests` compile with warnings on and warnings fatal. That's the second shared
helper in `Build.lua`, `ApplyStrictWarnings()`, called right after `ApplyDefaultProjectSettings()`:

```lua
function ApplyStrictWarnings()
    warnings "High"           -- -Wall   (clang/gcc) | /W4 (MSVC)
    externalwarnings "Off"    --                     | /external:W0
    fatalwarnings { "All" }   -- -Werror (clang/gcc) | /WX (MSVC)
end
```

It is deliberately *not* folded into `ApplyDefaultProjectSettings()`, which every project calls: third-party
source keeps whatever warning level its authors settled on. We don't patch spdlog or ImGui, and bumping a
dependency must not be able to break our build. `App` doesn't call it either — add the one line to
`App/Build-App.lua` if the game code should be held to the same bar.

Premake's portable verbs are used rather than raw flags because the same scripts generate an MSVC solution
*and* clang makefiles, and the literal flags don't translate: `-Wall` on MSVC (`/Wall`) means every
off-by-default warning including the ones the CRT headers trip. `warnings "High"` is the intended
equivalent. The two are not identical sets — MSVC's `/W4` includes C4100 (unreferenced formal parameter)
and C4456/C4458 (shadowing), which GCC/Clang put in `-Wextra` and `-Wshadow` respectively — so the Windows
build is the stricter of the two. `disablewarnings { "4100" }` inside the helper is the escape hatch if
that asymmetry ever becomes a nuisance.

**Vendored headers are the other half of this.** A third-party header included from one of our `.cpp`
files warns as if we had written it, and `/WX` would then fail our build over ImGui's code. So both
strict projects list every vendored and SDK path under `externalincludedirs` instead of `includedirs`:

```lua
includedirs { "Include", "Include/Core", ..., "Source" }   -- ours: warnings on, fatal
externalincludedirs { DepPath("spdlog", "include"), ... }  -- theirs: warnings off
```

That emits `-isystem` on GCC/Clang and `<ExternalIncludePath>` + `<ExternalWarningLevel>` on MSVC. Search
order is unchanged — both are searched *after* the normal include dirs, which is exactly where the vendored
entries already sat — so this is warning policy only, not a resolution change. **When you add a vendored
dependency to `Core` or `Tests`, put its path in `externalincludedirs`, not `includedirs`.**

## Box2D — the one project that isn't C++

Box2D 3.x (we vendor the [v3.1.1 release](https://github.com/erincatto/box2d/releases/tag/v3.1.1)) is a
rewrite of the old C++ 2.x line into **plain C**, so `Vendor/Build-Vendor.lua`'s `box2d` project is the
only one that overrides what `ApplyDefaultProjectSettings()` sets:

```lua
language "C"
cdialect "C17"   -- Box2D needs C17 for _Static_assert and anonymous unions
```

Three consequences worth knowing:

- **The workspace-wide MSVC options in `Build.lua` are C++-only.** `/EHsc` and `/Zc:__cplusplus` make MSVC
  emit `D9002 unknown option` on a C compiland, so the project calls `removebuildoptions` for those two
  under `filter "system:windows"`. `/Zc:preprocessor` and `/execution-charset:utf-8` are valid for C and
  stay. Premake emits the dialect as `<LanguageStandard_C>stdc17</LanguageStandard_C>`, which is separate
  from the `<LanguageStandard>stdcpp23</LanguageStandard>` that C++ projects use.
- **Consumers still `#include <box2d/box2d.h>` from C++ normally.** Every public entry point is declared
  `extern "C"` by `include/box2d/base.h`, so no `extern "C" { }` wrapper is needed on our side.
- **Floating-point contraction is disabled on GCC/Clang** (`-ffp-contract=off`), matching upstream's CMake.
  Box2D depends on strict IEEE 754 evaluation order for [cross-platform determinism](https://box2d.org/posts/2024/08/determinism/),
  which FMA contraction breaks. MSVC's default `/fp:precise` already disables it, so the flag is guarded
  by `filter "system:not windows"`.

Upstream's CMake options are mirrored at their defaults: SIMD **on** (SSE2, which is baseline on x64) and
`BOX2D_AVX2` **off**, so the binaries stay portable — define `BOX2D_AVX2` and add `/arch:AVX2` in the
project block if you ever want it. Box2D's asserts are gated by `NDEBUG`, which `ApplyDefaultProjectSettings()`
already defines for `Release`/`Dist`, so they're live in `Debug` only — the same policy as the ECS asserts.

Only `Core` sees Box2D today (`includedirs` + `links`). If a *public* `Core` header ever exposes Box2D
types (e.g. a `b2BodyId` on a physics component), `App` and `Tests` will need
`DepPath("box2d", "include")` on their include paths too — exactly how `Tests` already carries the
nvrhi/spdlog/Vulkan header paths for the types `Application.h` exposes.

## NVRHI — ported from CMake rather than built by it

[NVRHI](https://github.com/NVIDIA-RTX/NVRHI) (MIT) replaced `Ping`, Mupfel's own Vulkan wrapper. It
ships a CMake build, and this repo does not use it: `Vendor/Build-Vendor.lua` defines `nvrhi` and
`nvrhi_vk` as ordinary Premake static libs instead.

That is worth justifying, because "shell out to CMake from the setup script" is the obvious
alternative. It was rejected because the port turned out to be nearly free and the alternative is not:

- **Upstream's CMake generates nothing.** No `configure_file`, no `file(GENERATE)`, no
  `add_custom_command`, no generated headers. (This is exactly what makes Catch2 *not* portable this
  way — see the next section.) The only generated artifacts are CMake package config files, which a
  Premake build has no use for.
- **Its source lists are flat**, one per target, with no per-file properties.
- **Two compile definitions matter**, both Windows-only and both on `nvrhi_vk`:
  `VK_USE_PLATFORM_WIN32_KHR` and `NOMINMAX`. Everything else (`NVRHI_WITH_AFTERMATH`,
  `NVRHI_WITH_RTXMU`, `NVRHI_WITH_NVAPI`) is off and reaches the compiler as `=0`.

Against that, invoking CMake would add a hard CMake dependency to the setup scripts, a second build
system to keep in sync, and a manual mapping of our three configurations onto CMake's — `Dist` has no
CMake equivalent and would have to be aliased to `Release` by hand.

The target split mirrors upstream exactly (`nvrhi` = `src/common` + `src/validation`, `nvrhi_vk` =
`src/vulkan`) so that adding the D3D12 backend later is a new project in this file rather than a
reshuffle of an existing one. We build the Vulkan backend only.

Four things about these projects are load-bearing:

- **`cppdialect "C++17"`, overriding the workspace's C++23.** Same reasoning as `box2d` overriding
  `language`: we don't fix third-party code, so a dependency bump must not be able to break the build
  over a dialect we picked for our own sources. These are the only two projects that override
  `ApplyDefaultProjectSettings()`'s language settings.
- **`src/common/dxgi-format.cpp` is deliberately excluded.** Upstream compiles it into the D3D
  backends only; it does not build without the DirectX headers, which we don't vendor.
- **`src/validation/*.cpp` is compiled in.** Upstream gates it behind `NVRHI_WITH_VALIDATION` (default
  ON) with *no* matching `#define` — the validation layer is selected at runtime by wrapping a device
  in `nvrhi::validation::createValidationLayer()`, so compiling it costs nothing until it is used.
- **`VK_USE_PLATFORM_WIN32_KHR` is repeated in `Core` and `Tests`.** Upstream marks it `PUBLIC`, and
  it must stay that way here: it changes what `<vulkan/vulkan.h>` declares, so a TU that disagrees
  with the one that compiled `nvrhi_vk` is looking at a different Vulkan API. `NOMINMAX` rides along
  because that define is what drags in `windows.h`.

### The `vulkan.hpp` dispatcher rule

`src/vulkan/vulkan-backend.h` opens with

```cpp
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
```

This is the same class of hazard as `GLM_FORCE_DEPTH_ZERO_TO_ONE` (see `ApplyDefaultProjectSettings()`
in `Build.lua`): a macro that changes the meaning of a header, where two translation units disagreeing
is an ODR violation the linker resolves silently. Two rules follow:

1. Any engine TU that includes `<vulkan/vulkan.hpp>` must do so with
   `VULKAN_HPP_DISPATCH_LOADER_DYNAMIC` set to `1`. `Core/Build-Core.lua` sets it project-wide, so no
   `Core` TU can disagree with `nvrhi_vk` by forgetting a `#define`. This is the one half of the rule
   the build system can enforce.
2. **Exactly one** TU in the process may define the dispatcher storage
   (`VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE`). In a **static** build that TU has to be
   ours: NVRHI emits the storage only under `NVRHI_SHARED_LIBRARY_BUILD`
   (`src/vulkan/vulkan-device.cpp:29`), which `Vendor/Build-Vendor.lua` correctly never defines on a
   `StaticLib`. `Core/Source/Renderer/Renderer.cpp:19` is that TU.

The same `#if` also skips NVRHI's own `VULKAN_HPP_DEFAULT_DISPATCHER.init()`, so `Core` owns
initialisation as well: the three cumulative global/instance/device `init()` calls in
`NVRHIContext::Init`, fed from the entry points vk-bootstrap already holds. Defining the storage
without them links and then crashes on the first `vk::` call inside `createDevice`.

Rule 1 is load-bearing for rule 2. With `VULKAN_HPP_DISPATCH_LOADER_DYNAMIC` unset,
`VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE` expands to *nothing at all*, silently
(`vulkan_hpp_macros.hpp:328`), and `VULKAN_HPP_DEFAULT_DISPATCHER` resolves to the static loader,
which has no `init` member. `Reviews/NVRHI-Dispatcher.md` walks through both failure modes.

Defining `NVRHI_SHARED_LIBRARY_BUILD` on `nvrhi_vk` would restore both halves in one line, and is
wrong: `nvrhi.h:45` also keys `NVRHI_API` off it, flipping every public symbol to
`__declspec(dllexport)` on a static library.

NVRHI also does **not** define `VULKAN_HPP_NO_EXCEPTIONS`, so `vulkan.hpp` is compiled with exceptions
enabled — provided by the workspace-wide `/EHsc` in `Build.lua`.

### The SDK version floor

`Build.lua` reads `VK_HEADER_VERSION` out of `$VULKAN_SDK/Include/vulkan/vulkan_core.h` and aborts
generation if it is below `VulkanHeaderVersionFloor` (currently **357**, i.e. SDK 1.4.357). Two
dependencies impose it and both fail late and illegibly without the check:

- `vulkan-backend.h` has `#if (VK_HEADER_VERSION < 318) #error`, which only fires once MSBuild is
  already compiling `nvrhi_vk`.
- `vk-bootstrap` is generated against a specific header version — the tag in `Deps.vk_bootstrap`
  names it — and references structs and enum values older headers don't declare, producing a wall of
  "undeclared identifier" rather than one legible message.

The floor is the higher of the two. **Keep it and the `vk_bootstrap` pin in `Dependencies.lua` moving
together**; the NVRHI half only rises when upstream raises its own `#error`.

## vk-bootstrap — the part NVRHI deliberately doesn't do

NVRHI is a *rendering* hardware interface: resources, command lists, pipelines, and automatic
resource-state tracking. It explicitly does not create instances, physical devices, logical devices,
queues or swapchains, and has no window integration — `nvrhi::vulkan::createDevice` takes handles the
application must already have.

[vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap) (MIT) is the smallest thing that
produces those handles: one translation unit, C++17, no compile definitions. It resolves Vulkan entry
points by `dlopen`ing the loader at runtime rather than linking it, which has one build consequence —
**`Core` links `dl` on Linux on its behalf**, since a static lib carries no link dependencies of its
own. Windows needs no equivalent (`LoadLibrary` lives in `kernel32`, linked by default).

`Core` still links the Vulkan import library separately: `glfwCreateWindowSurface` and the
surface/present calls in our own device layer are ordinary prototypes resolved at link time.

## Catch2 — vendored as an amalgamation, not a source tree

Catch2 v3 is a compiled library, and its normal build path runs CMake first to generate
`catch_user_config.hpp` from a `.in` template — a configure step this Premake build has no equivalent
of. Upstream sidesteps that for exactly this case by publishing an **amalgamated** build with every
release: `catch_amalgamated.hpp` + `catch_amalgamated.cpp`, one header and one source file with the
config already baked in. `Deps.catch2` fetches those two release assets (that's what `single_files`
above exists for) and `Vendor/Build-Vendor.lua` compiles them as a plain static lib.

The one non-obvious flag is `DO_NOT_USE_WMAIN`:

```lua
defines { "DO_NOT_USE_WMAIN" }
```

`catch_amalgamated.cpp` supplies the test runner's `main()` — which is why `Tests/Source` has no entry
point of its own — but it emits `wmain()` instead when `_UNICODE` is defined, and
`ApplyDefaultProjectSettings()`'s `characterset "Unicode"` defines exactly that. MSVC decides between
the `mainCRTStartup` and `wmainCRTStartup` entry points by inspecting only the **object files on the
link line**, never the contents of a static lib, so it would pick `mainCRTStartup` and then fail with
an unresolved external `main`. Forcing the narrow-char entry point avoids the mismatch. (To take the
entry point back — e.g. to stand up engine state before the first assertion — define
`CATCH_AMALGAMATED_CUSTOM_MAIN` on the `catch2` project and write `main()` in `Tests` instead.)

Bumping Catch2 is a one-line version change in `Deps.catch2`'s two URLs; nothing else references the
version.

## Solution grouping

`Build.lua` wraps the includes so the generated solution mirrors the three tiers:

```lua
group "Vendor"
   include "Vendor/Build-Vendor.lua"   -- spdlog, imgui, nvrhi, nvrhi_vk, vk-bootstrap, box2d, catch2
group ""

for _, module in ipairs(SelectedModules()) do
   group (Modules[module].group)
      include (Modules[module].build_script)
   group ""
end
```

The loop covers `Core` (group `Engine`), `App` (ungrouped — it's the startproject) and `Tests` (group
`Tests`). Which group each lands in, and which file defines it, comes from the module table in
`Modules.lua`; that table is also what the `--modules` option filters, so a subset selection generates
a subset of these includes.

## Bugs fixed along the way

- **`glm` was never fetched.** `App/Build-App.lua` has always included `Vendor/Sources/glm-master/glm`,
  but no `Dependencies.lua` function ever downloaded it — it only worked because a copy already existed
  on disk. It's now a proper `Deps.glm` entry, fetched like everything else.
- **The GLFW download's `system:windows` guard was a no-op.** It used a Premake `filter(...)` call to
  gate `check_glfw()`, but `filter` only affects the currently active workspace/project configuration
  scope — and this code runs *before* `workspace "Mupfel"` is even declared, so the filter had no
  container to apply to. In practice this meant the Windows-only prebuilt GLFW binary zip was
  downloaded unconditionally, including on Linux (`Scripts/Setup-Linux.sh`). It's now a real
  `if os.target() == "windows" then ... end` check in `build_externals()`.
- **Fragile relative paths (`"../..."`) tied every path to a file's location in the directory tree.**
  Moving the vendor project definitions out of `Core/Ping.lua` into `Vendor/Build-Vendor.lua` would
  have silently broken every `"../Vendor/..."`-style path (wrong number of `../` for the new nesting
  depth). All paths now go through `DepPath()`/`%{wks.location}`, which is correct regardless of which
  directory the referencing script lives in.

## Verification

Regenerated the solution (`premake5 vs2026`) and built all configurations end-to-end
(`Vendor` → `Core` → `App`) with MSBuild: 0 errors, output binaries land in the same
`Binaries/<system>-<arch>/<config>/<project>/` layout as before.

For the NVRHI port specifically: solution generation succeeds, the SDK floor check was confirmed to
fire on an SDK below it and to pass above it, and `nvrhi`, `nvrhi_vk` and `vk-bootstrap` build in
**Debug, Release and Dist** with 0 warnings and 0 errors.

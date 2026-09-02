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
  Build-Vendor.lua         Builds vendored third-party static libs: spdlog, imgui, Logger, Ping, box2d,
                            catch2.
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
                         │ Vulkan SDK  │  (system dependency, VULKAN_SDK env var — not vendored)
                         └──────┬──────┘
                                │ headers
     ┌────────────┐            │            ┌────────────┐
     │   glfw3    │◄───────────┼────────────┤   spdlog   │  (no internal deps)
     │ (prebuilt, │  headers   │   headers  └─────┬──────┘
     │  Windows)  │            │                  │ headers
     └─────┬──────┘            │                  ▼
           │ headers      ┌────▼───┐        ┌────────────┐
           └─────────────►│ imgui  │        │   Logger   │  (split out of Ping's source
                           └────────┘        └─────┬──────┘   tree so Core and Ping can
                                                    │ link      each link it independently)
                                                    ▼
                                              ┌────────────┐
                            headers  ┌───────►│    Ping    │
                      (spdlog/glfw/  │        └─────┬──────┘
                     imgui/stb/vk)   │              │ link
                                     │              ▼
     ┌────────────┐             ┌────┴───────────────────┐
     │   box2d    ├────────────►│          Core           │  Mupfel's engine (Core/Source)
     │  (C17, no  │    link     │  links: Ping, Logger,    │  headers: nlohmann, ping, vulkan,
     │  int. deps)│             │  spdlog, imgui, glfw3,   │           glfw, spdlog, imgui,
     └────────────┘             │  vulkan, box2d          │           box2d
                                └────────────┬─────────────┘
                                             │ link
                                             ▼
                                ┌─────────────────────────┐
                                │           App            │  Game/editor (App/Source)
                                │  links: Core              │  headers: nlohmann, glm, ping,
                                │                            │           spdlog, vulkan
                                └─────────────────────────┘
```

`Tests` hangs off `Core` the same way `App` does — a `ConsoleApp` linking the engine, kept out of the
diagram above along with its catch2 framework rather than folded into it.

Header-only dependencies (no build project, just `includedirs`): **nlohmann/json**, **glm**,
**stb_image**. `stb_image` is only ever included by `Ping`; `nlohmann` is used by both `Core` (entity
serialization) and `App`; `glm` (math) is only used by `App` today. **catch2** is *not* header-only — see
"Catch2" below for why it gets a project of its own despite shipping as two files.

**Who links what:**

| Project  | Links against                                   | Also sees headers of (no link) |
|----------|--------------------------------------------------|---------------------------------|
| `spdlog` | —                                                  | —                                |
| `imgui`  | —                                                  | glfw                             |
| `Logger` | —                                                  | spdlog                           |
| `Ping`   | Logger                                             | spdlog, glfw, imgui, stb, vulkan |
| `box2d`  | —                                                  | —                                |
| `catch2` | —                                                  | —                                |
| `Core`   | Ping, Logger, spdlog, imgui, glfw3, vulkan, box2d  | nlohmann                         |
| `App`    | Core                                               | nlohmann, glm, ping, spdlog, vulkan |
| `Tests`  | Core, catch2                                       | + Core's header set (anything reaching Application.h) |

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
Ping/spdlog/Vulkan header paths for the types `Application.h` exposes.

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
   include "Vendor/Build-Vendor.lua"   -- spdlog, imgui, Logger, Ping, box2d, catch2
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

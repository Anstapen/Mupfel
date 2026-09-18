# Build system structure

How Mupfel's CMake build is organized: which libraries exist, what each one depends on, and which files
define them. The build is split into three tiers — **Vendor** (third-party code) / **Engine** (`Core`) /
**Application** (`App`) — with each tier's targets defined only inside that tier's own directory.

Migrated from Premake in 2026. A few rules below read as oddly specific; that is usually because they
encode something that failed once, and the reason is given where it matters.

## Building it

Everything goes through `CMakePresets.json`. There are no setup scripts.

```
cmake --preset windows            # Visual Studio 2026 solution in Build/windows
cmake --preset windows-ninja      # Ninja Multi-Config; use this one for CLion
cmake --preset linux              # Ninja Multi-Config

cmake --build --preset windows-debug        # or windows-release
ctest --preset windows-debug
```

Requirements: **CMake 4.0+**, the **Vulkan SDK** with `VULKAN_SDK` set, and on Linux **GCC 14+**
(`std::ranges::to`) plus `libglfw3-dev`. The `windows` preset needs Visual Studio 2026; CI uses
`ci-windows` instead, because the GitHub runner image ships VS 2022.

CMake 4.0 is the floor for two reasons: the `MSVC_RUNTIME_CHECKS` property (3.31) that box2d needs, and
the `Visual Studio 18 2026` generator.

> `windows-ninja` pins `CMAKE_C_COMPILER`/`CMAKE_CXX_COMPILER` to `cl` and clears `CC`/`CXX`. If those
> environment variables name another compiler, CMake honours them over `cl` and the build fails deep
> inside the MSVC standard library instead of at configure time.

**CLion**: only the `windows-ninja` profile works. `windows` and `ci-windows` use the Visual Studio
generator, which CLion cannot drive.

## Directory layout

```
CMakeLists.txt             Root: configurations, MSVC runtime/debug-info policy, subdirectories.
CMakePresets.json          Generators, build directories, CI presets.

cmake/
  Modules.cmake            MUPFEL_MODULES: which of core/app/tests this build contains.
  Dependencies.cmake       Where each third-party source lives and how to fetch it.
  MupfelSettings.cmake     mupfel_apply_default_settings / mupfel_apply_strict_warnings.
  VulkanSDK.cmake          Locates the SDK, asserts VK_HEADER_VERSION, publishes Mupfel::Vulkan.
  GLFW.cmake               Publishes Mupfel::GLFW (vendored on Windows, system lib on Linux).

Vendor/CMakeLists.txt      spdlog, imgui, nvrhi, nvrhi_vk, vk-bootstrap, box2d, catch2, plus
                            interface targets for the header-only deps.
Vendor/Sources/            Fetched source trees (gitignored).

Core/CMakeLists.txt        The "Core" engine static lib. Include/ is published, Source/ is internal.
App/CMakeLists.txt         The "App" executable (+ App/Shaders).
Tests/CMakeLists.txt       The "Tests" executable, Catch2-based.
```

Which modules are in the build comes from `MUPFEL_MODULES` (default `all`), closed over each module's
requirements:

```
cmake --preset windows -D MUPFEL_MODULES=app     # engine + game only
cmake --preset windows -D MUPFEL_MODULES=core    # engine alone
```

`App` and `Tests` both require `core`, so **`Core` is in every possible selection** — and so is every
vendored library it links, which is why `Vendor/CMakeLists.txt` only bothers gating `catch2`. An unknown
name aborts configuration with the valid list.

## Dependency graph

Three layers, bottom-up: the vendored libraries have no dependencies on each other except
`nvrhi_vk -> nvrhi`; `Core` links all of them; `App` and `Tests` link only `Core` (plus catch2, and a
little more for `Tests`, which is white-box). The **Vulkan SDK** is a system dependency found via
`VULKAN_SDK`, not vendored. **glfw3** is a prebuilt binary on Windows and the system library on Linux.

Header-only dependencies get interface targets rather than bare include paths, so consumers link a name
instead of repeating a path: **`Mupfel::nlohmann`**, **`Mupfel::glm`**, **`Mupfel::stb`**. `Core` owns
the single translation unit that defines `STB_IMAGE_IMPLEMENTATION`. **catch2** is *not* header-only —
see "Catch2" below.

**Who links what:**

| Target | `target_link_libraries` | Notes |
|---|---|---|
| `spdlog` | — | publishes its include dir |
| `imgui` | `Mupfel::GLFW` (PRIVATE, headers only) | publishes imgui + backends |
| `nvrhi` | — | publishes `NVRHI_WITH_AFTERMATH=0` |
| `nvrhi_vk` | `nvrhi`, `Mupfel::VulkanHeaders` (PUBLIC) | publishes `VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1`, and on Windows `VK_USE_PLATFORM_WIN32_KHR` + `NOMINMAX` |
| `vk-bootstrap` | `Mupfel::VulkanHeaders` (PUBLIC) | dlopens the loader; never links it |
| `box2d` | — | the one C target |
| `catch2` | — | only `Tests` links it |
| `Core` | **PUBLIC** `spdlog`, `Mupfel::nlohmann`<br>**PRIVATE** `nvrhi_vk`, `nvrhi`, `vk-bootstrap`, `imgui`, `box2d`, `Mupfel::Vulkan`, `Mupfel::GLFW`, `Mupfel::glm`, `Mupfel::stb`, `winmm` (Windows) | |
| `App` | `Core` | inherits exactly `Core/Include`, spdlog, nlohmann |
| `Tests` | `Core`, `catch2`, `nvrhi_vk`, `Mupfel::glm` | white-box: also adds `Core/Source` |

## Public and private: how App is kept away from the vendored libraries

`Core` is a static library, so a `PRIVATE` dependency still propagates on the **link line** (CMake
records it as `LINK_ONLY`) while its include directories and compile definitions do not. `App` therefore
links `nvrhi_vk.lib` without being able to `#include` an NVRHI header.

That is the whole mechanism. NVRHI, vk-bootstrap, the Vulkan SDK, GLFW, ImGui, Box2D and glm are
unreachable from application code by construction, not by keeping a list correct — `App`'s include path
comes out as exactly `App/Source`, `Core/Include`, spdlog, nlohmann.

Only three things are `PUBLIC`, and each is load-bearing:

- **`Core/Include`** — the published header root. `Core/Source` is internal; `App` cannot reach it.
- **spdlog** — `Core/Include/Core/Logger.h` names `spdlog::logger` in `SafeLoggerPtr`.
- **nlohmann json** — `FS/EntityFileManager.h` names `nlohmann::json` in its public `ComponentLoader`
  signature.

Both vendored ones are *deliberately published*, not leaked. Removing either breaks a public header.

`Core` also lists each public subdirectory (`Include/Core`, `Include/ECS`, …) as **PRIVATE**, because
engine code includes siblings by bare name (`#include "SubRenderer.h"`). All public header basenames are
unique, so the flattened search is unambiguous. They are PRIVATE so `App` cannot resolve a bare
`"SubRenderer.h"` of its own; path-qualifying those includes would let the entries be dropped.

Adding a vendored dependency that should stay hidden: put it in the `PRIVATE` half and nothing else is
needed.

## Dependencies

`cmake/Dependencies.cmake` is the single source of truth. Each dependency is declared once:

```cmake
mupfel_declare_dep(spdlog
    RELPATH "Vendor/Sources/spdlog-1.17.0"
    URL     "https://github.com/gabime/spdlog/archive/refs/tags/v1.17.0.zip")
```

which publishes `MUPFEL_DEP_spdlog_DIR` for everything else to compose against. Bumping a version is one
line. Which fields are set decides how it is fetched:

| Fields | Meaning |
|---|---|
| `URL` + `RELPATH` | zip archive, extracted into `Vendor/Sources/` |
| `+ SINGLE_FILE` | one loose file (`json.hpp`, `stb_image.h`) |
| `SINGLE_FILES` | several loose files (Catch2's amalgamation) |

`SINGLE_FILES` checks each file individually, so an interrupted run resumes rather than leaving a
half-populated directory that reads as "present". A failed download is deleted for the same reason —
`file(DOWNLOAD)` writes a file even on failure, and it would satisfy the presence check forever after.

**Upstream `CMakeLists.txt` files are deliberately never used.** Every vendored library is rebuilt as one
of our own targets, so the fetch is a plain download-and-extract rather than `FetchContent_MakeAvailable`:
we want the sources on disk, not their option sets, install rules and dependency resolution in our graph.

## Shared target settings

`mupfel_apply_default_settings(<target>)` carries output directories, the per-configuration
`DEBUG`/`RELEASE` defines, and the MSVC define and option sets. It does **not** set the language
standard — that is per-target (box2d is C17, NVRHI and vk-bootstrap are C++17, ours are C++23).

`/EHsc` and `/Zc:__cplusplus` are scoped with `$<COMPILE_LANGUAGE:CXX>`, which MSVC rejects on a C
compiland; that is what keeps box2d working without an explicit opt-out.

### Configurations, and Release symbols

Two configurations, `Debug` and `Release`, both ones CMake already knows. Release is optimized **with**
symbols, which takes two halves — miss either and you get a Release you cannot debug:

- **Compiler**: the debug information format below, plus `-g` on GCC.
- **Linker**: `/DEBUG` produces the `.pdb` for the executable. Without it you get an executable with an
  **empty Debug Directory** — full debug info in the objects, no PDB reference in the binary, and a
  debugger that cannot bind a single breakpoint. `/OPT:REF` and `/OPT:ICF` are restored alongside it on
  Release, because the linker disables both as soon as `/DEBUG` appears.

  **Both** configurations state `/DEBUG` explicitly. Release needs it because CMake only supplies it for
  `Debug` and `RelWithDebInfo`. Debug states it too because CMake's value is a *cache* entry: it is
  whatever the first configure of a build directory produced and it never updates afterwards. A
  directory first configured with a Clang toolchain, or with `LDFLAGS` set, keeps an empty
  `CMAKE_EXE_LINKER_FLAGS_DEBUG` forever — even after switching to `cl`. That is not hypothetical; it is
  what silently broke debugging in CLion while the Visual Studio generator's directory, configured by a
  different CMake, worked fine. Verify with `dumpbin /HEADERS App.exe`: the Debug Directories table must
  contain an `RSDS` entry naming the PDB.

Debug information is **Embedded (`/Z7`)**, not `ProgramDatabase (/Zi)`, and that is load-bearing. With
`/Zi` each static library writes its own `.pdb` that the linker must find again; because every target has
its own output directory it finds none, and emits `LNK4099` per object — every `Core` object contributing
no symbols to `Tests.exe`, announced as a warning nobody reads. `/Z7` leaves nothing to lose track of.
Cost: the static libs roughly double in size. They are gitignored build artifacts and the shipped
executable is byte-identical either way.

The Premake build also had a `Dist` (Release codegen, symbols stripped). It was dropped because nothing
used it. Reinstating a custom configuration means defining `CMAKE_{C,CXX}_FLAGS_DIST`, the four
linker-flag variants and `CMAKE_MAP_IMPORTED_CONFIG_DIST` explicitly — an undefined one silently compiles
unoptimized with no `NDEBUG`.

## Warnings: strict for our code, silent for vendored code

`Core` and `Tests` call `mupfel_apply_strict_warnings()`; nothing else does. Third-party source keeps
whatever warning level its authors settled on — we don't patch spdlog or ImGui, and a dependency bump
must not be able to break our build. `App` doesn't call it either; add the line if the game code should
be held to the same bar.

The other half is include *paths*: a third-party header included from one of our `.cpp` files warns as if
we had written it. Every vendored target publishes its include directory as `SYSTEM`, which emits
`-isystem` / `/external:I`, compiled at the `/external:W0` level the strict helper sets. **When you add a
vendored dependency, mark its interface include directory `SYSTEM`** — consumers inherit it, so getting
it right once fixes it everywhere.

### Warning parity: MSVC `/W4` is the reference

`-Wall` and `/W4` are different sets in both directions, and with warnings fatal each extra check on one
side is a build that fails on that platform only. `/W4` (plus C5038, which `/W4` leaves off) is the
reference, because Windows is where the code is written. GCC does **not** get `-Wall`: it gets an explicit
list of counterparts, so a GCC upgrade that adds checks to `-Wall` changes nothing here.

| GCC flag | MSVC `/W4` | GCC also reports (MSVC accepts) |
|---|---|---|
| `-Wreorder` | C5038 | |
| `-Wunused-parameter` | C4100 | |
| `-Wunused-variable` | C4101, C4189 | unused `static` variables at namespace scope |
| `-Wunused-function` | C4505 | unused functions in an anonymous namespace |
| `-Wunused-label` | C4102 | |
| `-Wunused-value` | C4553 | |
| `-Wuninitialized` | C4700 | |
| `-Wempty-body` | C4390 | |
| `-Wsign-compare` | C4018, C4389 | `int` vs `size_t`, e.g. `i < v.size()` |
| `-Wformat` | C4477 | |
| `-Wbool-compare` | C4806 | |
| `-Wunknown-pragmas` | C4068 | |
| `-Winfinite-recursion` | C4717 | |
| `-Wdelete-non-virtual-dtor` | C5205 | deleting a *non-abstract* polymorphic class |
| `-Wconversion` | C4244, C4267, C4305 | `long` → `int` (64-bit on Linux, 32-bit on Windows) |

Deliberately **not** enabled, although `/W4` has a counterpart, because GCC's version also covers code
MSVC accepts: `-Wparentheses`, `-Wshadow`, `-Wsign-conversion`, and `-Wmaybe-uninitialized` (optimizer-
dependent, with long-standing false positives on `std::optional`/`std::expected`). The last one plus
`-Wformat-overflow` / `-Wformat-truncation` are disabled *explicitly*, because `-Wuninitialized` and
`-Wformat` imply them.

The list is scoped to GNU rather than "not MSVC" because clang rejects `-Wbool-compare` as unknown, which
`-Werror` makes fatal; a clang build keeps plain `-Wall`.

## Box2D — the one target that isn't C++

Box2D 3.x ([v3.1.1](https://github.com/erincatto/box2d/releases/tag/v3.1.1)) is a rewrite of the C++ 2.x
line into **plain C**, needing C17 for `_Static_assert` and anonymous unions.

- **Consumers still `#include <box2d/box2d.h>` from C++ normally** — every public entry point is declared
  `extern "C"` by `include/box2d/base.h`.
- **`C_EXTENSIONS ON` on Linux** (upstream's `C_EXTENSIONS YES`). Strict `-std=c17` defines
  `__STRICT_ANSI__`, so glibc hides POSIX and `src/timer.c` fails on `CLOCK_MONOTONIC`. MSVC knows only
  stdc11/stdc17/stdclatest, so it gets `C_EXTENSIONS OFF`.
- **Debug keeps its optimizer**, because an unoptimized solver makes the game too slow to debug. On MSVC
  that forces clearing `MSVC_RUNTIME_CHECKS`: `/RTC1` with `/O2` is a hard `error D8016`, and there is no
  negative form of `/RTC1` to pass as a compile option.
- **FP contraction is disabled**, because Box2D depends on strict IEEE 754 evaluation order for
  [determinism](https://box2d.org/posts/2024/08/determinism/). Keyed on the **compiler**, not the
  platform, and the spellings are not interchangeable — two of the three failure modes are silent:

  | | |
  |---|---|
  | MSVC | nothing; `/fp:precise` is the default and already disables it |
  | clang-cl | `/clang:-ffp-contract=off` — a bare `-ffp-contract=off` is *ignored* |
  | clang/gcc | `-ffp-contract=off` |

  Contraction cannot actually occur on the SSE2 baseline (no FMA instruction to contract into), so this
  is insurance for anyone enabling `BOX2D_AVX2` later — precisely when a silently-dropped flag would
  start costing determinism.

Upstream's options are mirrored at their defaults: SIMD **on** (SSE2, baseline on x64), `BOX2D_AVX2`
**off** so binaries stay portable. Asserts are gated by `NDEBUG`, so they are live in Debug only.

Only `Core` sees Box2D, and links it `PRIVATE`. If a *public* `Core` header ever exposes a Box2D type,
moving `box2d` to the `PUBLIC` half is the whole change.

## NVRHI — ported from CMake rather than built by it

[NVRHI](https://github.com/NVIDIA-RTX/NVRHI) (MIT) replaced `Ping`, Mupfel's own Vulkan wrapper. It ships
a CMake build, and this repo does not use it even now that the build is itself CMake:
`Vendor/CMakeLists.txt` defines `nvrhi` and `nvrhi_vk` as static-lib targets of our own. That is worth
justifying, because adopting upstream's build is the obvious alternative:

- **Upstream's CMake generates nothing.** No `configure_file`, no generated headers. (This is exactly
  what makes Catch2 *not* portable this way — see below.) Its only generated artifacts are package config
  files, for consumers who `find_package()` it.
- **Its source lists are flat**, one per target, with no per-file properties.
- **Two compile definitions matter**, both Windows-only and both on `nvrhi_vk`:
  `VK_USE_PLATFORM_WIN32_KHR` and `NOMINMAX`. Everything else is off and reaches the compiler as `=0`.

Against that, adopting it would mean inheriting its option set, its install rules and its own dependency
resolution, and keeping our configuration and warning policy in sync with whatever it does by default.

The target split mirrors upstream exactly (`nvrhi` = `src/common` + `src/validation`, `nvrhi_vk` =
`src/vulkan`) so adding the D3D12 backend later is a new target rather than a reshuffle. `src/validation`
is compiled in unconditionally: the validation layer is selected at runtime by wrapping a device in
`nvrhi::validation::createValidationLayer()`, so it costs nothing until used. `src/common/dxgi-format.cpp`
is deliberately absent — upstream compiles it into the D3D backends only.

### The `vulkan.hpp` dispatcher rule

`src/vulkan/vulkan-backend.h` opens with

```cpp
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
```

Same class of hazard as `GLM_FORCE_DEPTH_ZERO_TO_ONE`: a macro that changes the meaning of a header,
where two translation units disagreeing is an ODR violation the linker resolves silently. Two rules:

1. Any engine TU that includes `<vulkan/vulkan.hpp>` must do so with
   `VULKAN_HPP_DISPATCH_LOADER_DYNAMIC` set to `1`. `nvrhi_vk` defines it **PUBLIC**, so every consumer
   inherits it from the library whose header forces it and no TU can disagree by forgetting a `#define`.
   This is the half a build system can enforce.
2. **Exactly one** TU may define the dispatcher storage
   (`VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE`). In a **static** build that has to be ours:
   NVRHI emits it only under `NVRHI_SHARED_LIBRARY_BUILD` (`src/vulkan/vulkan-device.cpp:29`), which
   `Vendor/CMakeLists.txt` correctly never defines. `Core/Source/Renderer/Renderer.cpp:19` is that TU.

The same `#if` skips NVRHI's own `VULKAN_HPP_DEFAULT_DISPATCHER.init()`, so `Core` owns initialisation
too: the three cumulative global/instance/device `init()` calls in `NVRHIContext::Init`, fed from the
entry points vk-bootstrap already holds. Defining the storage without them links and then crashes on the
first `vk::` call inside `createDevice`.

Rule 1 is load-bearing for rule 2. With `VULKAN_HPP_DISPATCH_LOADER_DYNAMIC` unset,
`VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE` expands to *nothing at all*, silently
(`vulkan_hpp_macros.hpp:328`), and `VULKAN_HPP_DEFAULT_DISPATCHER` resolves to the static loader, which
has no `init` member. `Reviews/NVRHI-Dispatcher.md` walks through both failure modes.

Defining `NVRHI_SHARED_LIBRARY_BUILD` on `nvrhi_vk` would restore both halves in one line, and is wrong:
`nvrhi.h:45` also keys `NVRHI_API` off it, flipping every public symbol to `__declspec(dllexport)` on a
static library.

NVRHI does **not** define `VULKAN_HPP_NO_EXCEPTIONS`, so `vulkan.hpp` compiles with exceptions enabled —
provided by the `/EHsc` that `mupfel_apply_default_settings()` puts on every C++ target.

### The SDK version floor

`cmake/VulkanSDK.cmake` reads `VK_HEADER_VERSION` out of `$VULKAN_SDK/Include/vulkan/vulkan_core.h` and
aborts configuration below `MUPFEL_VULKAN_HEADER_VERSION_FLOOR` (**357**, i.e. SDK 1.4.357). Two
dependencies impose it and both fail late and illegibly without the check:

- `vulkan-backend.h` has `#if (VK_HEADER_VERSION < 318) #error`, which only fires once the build is
  already compiling `nvrhi_vk`.
- `vk-bootstrap` is generated against a specific header version (`MUPFEL_VK_BOOTSTRAP_VERSION` names it)
  and references structs and enum values older headers don't declare, producing a wall of "undeclared
  identifier" rather than one legible message.

The floor is the higher of the two. **Keep it and the `vk_bootstrap` pin in `cmake/Dependencies.cmake`
moving together**; the NVRHI half only rises when upstream raises its own `#error`.

## vk-bootstrap — the part NVRHI deliberately doesn't do

NVRHI is a *rendering* hardware interface: resources, command lists, pipelines, automatic resource-state
tracking. It does not create instances, devices, queues or swapchains, and has no window integration —
`nvrhi::vulkan::createDevice` takes handles the application must already have.

[vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap) (MIT) is the smallest thing that produces
them: one translation unit, C++17, no compile definitions. It resolves Vulkan entry points by `dlopen`ing
the loader rather than linking it, which has one build consequence: **`Mupfel::GLFW` carries `dl` on
Linux** on its behalf, since a static lib carries no link dependencies of its own. Windows needs no
equivalent (`LoadLibrary` lives in `kernel32`).

This is why vk-bootstrap takes `Mupfel::VulkanHeaders` (headers only) rather than `Mupfel::Vulkan`.
`Core` links the full `Mupfel::Vulkan`, because `glfwCreateWindowSurface` and the surface/present calls in
our own device layer are ordinary prototypes resolved at link time.

## Catch2 — vendored as an amalgamation, not a source tree

Catch2 v3 is a compiled library whose normal build path runs CMake first to generate
`catch_user_config.hpp` from a `.in` template — which would mean adopting upstream's whole build, the
thing this repo avoids for every vendored library. Upstream sidesteps that by publishing an
**amalgamated** build with every release: `catch_amalgamated.hpp` + `catch_amalgamated.cpp`, with the
config baked in. The `catch2` declaration fetches those two release assets (that's what `SINGLE_FILES`
exists for) and `Vendor/CMakeLists.txt` compiles them as a plain static lib.

The one non-obvious flag is `DO_NOT_USE_WMAIN`. `catch_amalgamated.cpp` supplies the test runner's
`main()` — which is why `Tests/Source` has no entry point — but emits `wmain()` instead when `_UNICODE`
is defined, which `mupfel_apply_default_settings()` does on MSVC. MSVC picks between `mainCRTStartup` and
`wmainCRTStartup` by inspecting only the **object files on the link line**, never inside a static lib, so
it would pick `mainCRTStartup` and fail with an unresolved `main`. (To take the entry point back — e.g.
to stand up engine state before the first assertion — define `CATCH_AMALGAMATED_CUSTOM_MAIN` on `catch2`
and write `main()` in `Tests`.)

Bumping Catch2 is a one-line change to its two URLs.

## Known gaps

- **The Linux build has not been run since the migration.** The workflow is ported and the toolchain
  logic is unchanged in substance, but CI is the first real check.

- **Premake leftovers are still in the tree**: `Scripts/` (three `.bat` files still invoking `premake5`,
  one gutted to a stub) and `Vendor/Binaries/Premake/`. Nothing references them. `.gitignore` also still
  carries Premake-era entries (`Makefile`, `*.ninja`, `!Vendor/Binaries`) and does not ignore
  `cmake-build-*/`, which CLion creates if a non-preset profile is ever used.

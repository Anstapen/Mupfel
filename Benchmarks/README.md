# Benchmarks

Microbenchmarks for Mupfel's hot CPU paths — the ECS `Registry`, `View` iteration, component access,
and `ParallelForEach` — built on the header-only [nanobench](https://github.com/martinus/nanobench)
framework (pinned to v4.3.11, vendored via `Dependencies.lua` like every other third-party dep).

This is the fourth build tier alongside `Vendor`, `Core`, and `App` (see `BUILD.md`). The `Benchmarks`
project is a `ConsoleApp` that links `Core`; it is generated into the solution by Premake automatically.

## Why this can run headless

`Registry`, `View`, `CPUComponentArray`, and the component-access paths need nothing but an
`EventSystem` and a `Registry` — both plain host-memory objects. No window, no Vulkan device, and no
`Application` are required, so each benchmark builds its own isolated ECS (`MupfelBench::World`).

The **one** exception is `ParallelForEach`: its template body calls `Application::GetCurrentThreadPool()`,
so that benchmark forces construction of the `Application` singleton. That only *default-constructs* it
(`Application::Init()` is never called), which just spins up the worker threads — no window is opened
and no Vulkan device is created. The singleton is primed once, off the clock, before timing begins.

## Building and running

The project is picked up by the normal setup flow. From `Scripts/`:

```
Setup-Windows.bat      # regenerates the VS solution incl. Benchmarks/Benchmarks.vcxproj
```

Then build the `Benchmarks` project (MSBuild or from the IDE) and run the executable:

```
Binaries/<system>-<arch>/Release/Benchmarks/Benchmarks.exe
```

Command-line flags:

| Flag           | Effect                                                                    |
|----------------|---------------------------------------------------------------------------|
| `--csv <path>` | also write machine-readable CSV results (for regression tracking)         |
| `-h`, `--help` | print usage                                                               |

> **Build in `Release` or `Dist`.** `Debug` is unoptimized and enables the ECS asserts, so its numbers
> are meaningless — the program prints a warning if run as a `Debug` build.

## Reliability model

Getting *trustworthy* numbers is the whole point, so the harness leans on nanobench's statistics rather
than raw `std::chrono` timing:

- **Warmup + minimum epoch time.** `BenchCommon.h::ApplyDefaults()` sets `warmup(20)` and
  `minEpochTime(50ms)` on every bench, so even the fastest cases accumulate hundreds of iterations per
  epoch. nanobench then reports the **median** across epochs plus an **err%** (median absolute
  percentage error). Aim for `err% < 5%`; if a row shows a `〰️` "Unstable" marker, raise the epoch
  budget. Runtime vs. stability is that single dial in `ApplyDefaults`.
- **Dead-code elimination is prevented** with `ankerl::nanobench::doNotOptimizeAway(...)` on every
  accumulated result; benchmarks that mutate components (lifecycle, ParallelForEach) rely on the
  observable writes into the registry.
- **`batch(N)`** is set to the number of entities/ops processed per invocation, so results are reported
  per entity/op (`ns/entity`, `entity/s`) and are comparable across different entity counts.
- **Balanced round-trips** for the mutating benchmarks (create *then* destroy; add *then* remove) keep
  the registry at steady state across epochs, and a per-invocation `events.Update()` drains the
  immediate events those paths fire so the event buffers don't grow unbounded during a run.

For the tightest numbers: build `Dist`, close other applications, and (optionally) pin the process to a
core and disable CPU frequency scaling / turbo so the clock is steady.

## What's covered

| File                        | Group                                                                    |
|-----------------------------|--------------------------------------------------------------------------|
| `Bench_View.cpp`            | single-component view scaling; match-density sweep; base-component ordering (rarest component should come first) |
| `Bench_ComponentAccess.cpp` | `GetComponent`/`HasComponent`, sequential vs. random handle order        |
| `Bench_Lifecycle.cpp`       | `CreateEntity`/`DestroyEntity`, `AddComponent`/`RemoveComponent` round-trips |
| `Bench_ParallelForEach.cpp` | `ParallelForEach` vs. single-threaded `View`, across entity counts       |

## Adding a benchmark

1. Write a group function in a new or existing `Bench_*.cpp`:
   ```cpp
   #include "BenchCommon.h"
   #include "Benchmarks.h"
   using namespace Mupfel;

   void MupfelBench::RunMyBenchmarks(std::ostream* csv)
   {
       World world;
       Populate(world, 50000);

       ankerl::nanobench::Bench bench;
       ApplyDefaults(bench).title("My thing").unit("entity");

       bench.batch(50000).run("does the thing", [&] {
           float sum = 0.0f;
           for (auto [e, t] : world.registry.view<Transform>())
               sum += t.pos_x;
           ankerl::nanobench::doNotOptimizeAway(sum);
       });

       RenderCsv(bench, csv);
   }
   ```
2. Declare it in `Benchmarks.h` and call it from `main.cpp`.

New `.cpp` files under `Benchmarks/Source/` are globbed automatically by `Build-Benchmarks.lua` — no
build-file edit needed, but re-run the setup script so Premake regenerates the project.

#pragma once
#include <iosfwd>

// Each benchmark group lives in its own .cpp and is invoked from main(). When `csv` is non-null,
// the group also appends machine-readable CSV rows to it (see BenchCommon.h / RenderCsv).
namespace MupfelBench {

void RunViewBenchmarks(std::ostream* csv);
void RunComponentAccessBenchmarks(std::ostream* csv);
void RunLifecycleBenchmarks(std::ostream* csv);
void RunParallelForEachBenchmarks(std::ostream* csv);

} // namespace MupfelBench

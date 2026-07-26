// Entry point for the Mupfel ECS microbenchmarks.
//
// Runs every benchmark group and prints a markdown results table per group to stdout. Pass
// `--csv <path>` to additionally dump machine-readable results for regression tracking.
//
// Reliability: build the Benchmarks project in Release or Dist. Debug builds are unoptimized and
// enable the ECS asserts, so their numbers mean nothing (a warning is printed below in that case).

#include "Benchmarks.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

int main(int argc, char** argv)
{
	std::string csv_path;

	for (int i = 1; i < argc; ++i)
	{
		if (std::strcmp(argv[i], "--csv") == 0 && i + 1 < argc)
		{
			csv_path = argv[++i];
		}
		else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0)
		{
			std::cout << "Mupfel ECS benchmarks\n"
						 "  --csv <path>   also write machine-readable CSV results (for regression tracking)\n"
						 "  -h, --help     show this help\n";
			return 0;
		}
		else
		{
			std::cerr << "Unknown argument: " << argv[i] << " (try --help)\n";
			return 1;
		}
	}

	std::unique_ptr<std::ofstream> csv_file;
	if (!csv_path.empty())
	{
		csv_file = std::make_unique<std::ofstream>(csv_path, std::ios::trunc);
		if (!*csv_file)
		{
			std::cerr << "Could not open CSV output file: " << csv_path << "\n";
			return 1;
		}
	}
	std::ostream* csv = csv_file ? csv_file.get() : nullptr;

#ifdef DEBUG
	std::cout << "\n[!] This is a DEBUG build - the numbers below are NOT representative.\n"
				 "    Build the Benchmarks project in Release or Dist for meaningful results.\n\n";
#endif

	MupfelBench::RunViewBenchmarks(csv);
	MupfelBench::RunComponentAccessBenchmarks(csv);
	MupfelBench::RunLifecycleBenchmarks(csv);
	MupfelBench::RunParallelForEachBenchmarks(csv);

	if (csv)
		std::cout << "\nCSV results written to " << csv_path << "\n";

	return 0;
}

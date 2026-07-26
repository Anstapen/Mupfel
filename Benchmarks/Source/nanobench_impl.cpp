// The single translation unit that compiles nanobench's implementation.
//
// nanobench is header-only: every other .cpp includes <nanobench.h> for declarations, but exactly
// one TU in the program must define ANKERL_NANOBENCH_IMPLEMENT before including it so the definitions
// are emitted here (and nowhere else, to avoid duplicate-symbol link errors).
#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

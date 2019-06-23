#ifndef _TESTS

#define DECLARE_EXTERN_TESTBENCH_VARS
#define COUNT_BLOCKS 1
#include "testbench.h"

#include "benchmark.h"
#include "tree.h"
#include "random_gen.h"
#include <LEDA/core/impl/ab_tree.h>
#include <LEDA/core/dictionary.h>
#include <iostream>

AggregateTimer timer;
BenchmarkResults bench;

int main() {
	bench.SetTextOutput(true);
	runFullBench(1);
	return 0;
}
#endif
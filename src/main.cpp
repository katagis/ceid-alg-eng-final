#ifndef _TESTS

#define DECLARE_EXTERN_TESTBENCH_VARS
#define COUNT_BLOCKS 1
#include "testbench.h"

#include "tree.h"
#include "random_gen.h"
#include <LEDA/core/impl/ab_tree.h>
#include <LEDA/core/dictionary.h>
#include <iostream>
#include <unordered_set>

AggregateTimer timer;
Benchmark bench;

//constexpr int NodeSize = 338; // blocksize
constexpr int NodeSize = 128;

using LedaTree = leda::dictionary<int, int*, leda::ab_tree>;
using ImplTree = Tree<int, int, NodeSize>;

void add_no_bench(LedaTree& leda, ImplTree& impl, int N, int seed, std::vector<int*>& outPtrs) {
	
	rd::seed(seed);

	std::vector<int> numbers;
	numbers.reserve(N);
	outPtrs.reserve(outPtrs.size() + N);

	for (int i = 0; i < N; ++i) {
		int num = rd::get();
		numbers.push_back(num);
		outPtrs.push_back(new int(num));
	}

	for (int i = 0; i < N; ++i) {
		leda.insert(numbers[i], outPtrs[i]);
	}

	for (int i = 0; i < N; ++i) {
		impl.set(numbers[i], outPtrs[i]);
	}
}

void add_test(LedaTree& leda, ImplTree& impl, int N, int seed, std::vector<int*>& outPtrs) {
	rd::seed(seed);

	std::vector<int> numbers;

	numbers.reserve(N);
	outPtrs.reserve(outPtrs.size() + N);

	for (int i = 0; i < N; ++i) {
		int num = rd::get();
		numbers.push_back(num);
		outPtrs.push_back(new int(num));
	}

	bench.StartTest();
	for (int i = 0; i < N; ++i) {
		leda.insert(numbers[i], outPtrs[i]);
	}
	bench.StopLeda();

	bench.StartTest();
	for (int i = 0; i < N; ++i) {
		impl.set(numbers[i], outPtrs[i]);
	}
	bench.StopImpl();
	bench.PrintLast({ TestType::Add }, "Add " + std::to_string(N / 1000) + "k");
}

void get_test(LedaTree& leda, ImplTree& impl, int N, int seed) {
	rd::seed(seed);

	std::vector<int> numbers;
	for (int i = 0; i < N; ++i) {
		numbers.push_back(rd::get());
	}

	int ledaR = 0;
	int implR = 0;
	bench.StartTest();
	for (int number : numbers) {
		leda::dic_item r = leda.lookup(number);
		if (r) {
			ledaR ^= *leda.inf(r);
		}
	}
	bench.StopLeda();

	bench.StartTest();
	for (int number : numbers) {
		int* num;
		if (impl.get(number, num)) {
			implR ^= *num;
		}
	}
	bench.StopImpl();
	bench.PrintLast({ TestType::Get }, "Get " + std::to_string(N / 1000) + "k");

	if (ledaR != implR) {
		std::cout << "comparision resulted in differences.\n";
	}
}

void delete_test(LedaTree& leda, ImplTree& impl, int N, int seed) {
	rd::seed(seed);
	std::vector<int> numbers;
	for (int i = 0; i < N; ++i) {
		numbers.push_back(rd::get());
	}
	bench.StartTest();
	for (int number : numbers) {
		leda.undefine(number);
	}
	bench.StopLeda();

	bench.StartTest();
	for (int number : numbers) {
		impl.remove(number);
	}
	bench.StopImpl();
	bench.PrintLast({ TestType::Del }, "Del " + std::to_string(N / 1000) + "k");
}

void delete_ex(LedaTree& leda, ImplTree& impl, unsigned int N, int seed) {
	rd::seed(seed);

	if (N > (impl.size() * 3) / 4) {
		std::cerr << "N is too big at delete exact\n";
		return;
	}

	std::vector<int> numbers;
	for (unsigned int i = 0; i < N; ) {
		int number = rd::get();
		if (impl.find(number).exists) {
			numbers.push_back(rd::get());
			++i;
		}
	}

	bench.StartTest();
	for (int number : numbers) {
		leda.undefine(number);
	}
	bench.StopLeda();

	bench.StartTest();
	for (int number : numbers) {
		impl.remove(number);
	}
	bench.StopImpl();
	bench.PrintLast({ TestType::Del }, "Del Exact " + std::to_string(N / 1000) + "k");
}

void iterate_all(LedaTree& leda, ImplTree& impl) {

	int count = impl.size();

	int ledaR = 0;
	int implR = 0;

	bench.StartTest();
	int n;
	forall_defined(n, leda) {
		ledaR ^= n;
	}
	bench.StopLeda();

	bench.StartTest();
	for (Iterator it = impl.first(); it.isValid(); ++it) {
		implR ^= it.key();
	}
	bench.StopImpl();

	if (ledaR != implR) {
		std::cout << "iteration resulted in differences.\n";
	}
	bench.PrintLast({ TestType::Iterate }, "Iter " + std::to_string(count / 1000000) + "m");
}

void add_items(LedaTree& leda, ImplTree& impl, int count) {
	for (int i = 0; i < count; ++i) {
		leda.insert(i, nullptr);
		impl.set(i, nullptr);
	}
}

int main() {
	LedaTree ledaDic(NodeSize / 2, NodeSize);
	ImplTree implDic;
	std::cout << "Memory size of Node: " << sizeof(ImplTree::TNode) << "\n";

	rd::setMax(2 * 1000 * 1000);

	std::vector<int*> ptrs;


	int seed = 0;
#ifndef _DEBUG
	add_test	(ledaDic, implDic, 1000000, ++seed, ptrs);
	get_test	(ledaDic, implDic, 1000000, ++seed);
	delete_ex   (ledaDic, implDic,  500000, ++seed); 

	add_test	(ledaDic, implDic, 1000000, ++seed, ptrs);
	get_test	(ledaDic, implDic, 1000000, ++seed);
	delete_test (ledaDic, implDic, 1000000, ++seed);
	add_test	(ledaDic, implDic,  500000, ++seed, ptrs);
	get_test	(ledaDic, implDic,  500000, ++seed);
	delete_test (ledaDic, implDic,  500000, ++seed);

	add_items   (ledaDic, implDic, 50 * 1000 * 1000);
	iterate_all (ledaDic, implDic);
	delete_ex   (ledaDic, implDic, 500000, ++seed);
#else // in debug just run some basic stuff because all the tests take too much time
	add_test(ledaDic, implDic, 2000000, 0, ptrs);
	delete_test(ledaDic, implDic, 1000000, 11);
	get_test(ledaDic, implDic, 2000000, 1);
#endif

	for (auto p : ptrs) {
		delete p;
	}

	bench.Print();
	timer.Print("Generic Timer");

	return 0;
}
#endif
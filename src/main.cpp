#include <iostream>
#include "testbench.h"

AggregateTimer Timer;
#define NDEBUG
#include "node.h"

#include <unordered_set>

#ifndef _TESTS

#include <LEDA/core/impl/ab_tree.h>
#include <LEDA/core/dictionary.h>



//constexpr int NodeSize = 338; // blocksize
constexpr int NodeSize = 16;

using LedaTree = leda::dictionary<int, int*, leda::ab_tree>;
using ImplTree = Tree<int, int, NodeSize>;

Benchmark bench;

constexpr int ModCount = 1000000;

void add_test(LedaTree& leda, ImplTree& impl, int N, int seed) {
	std::vector<int> numbers;
	for (int i = 0; i < N; ++i) {
		numbers.push_back(std::rand() % ModCount);
	}

	std::srand(seed);
	bench.StartTest();
	for (int number : numbers) {
		leda.insert(number, new int(number));
	}
	bench.SwitchTest();

	std::srand(seed);
	bench.StartTest();
	for (int number : numbers) {
		impl.set(number, new int(number));
	}
	bench.StopTest();
	bench.PrintLast({ TestType::Add }, "Add " + std::to_string(N / 1000) + "k");
}

void get_test(LedaTree& leda, ImplTree& impl, int N, int seed) {

	std::vector<int> numbers;
	for (int i = 0; i < N; ++i) {
		numbers.push_back(std::rand() % ModCount);
	}

	int ledaR = 0;
	int implR = 0;
	std::srand(seed);
	bench.StartTest();
	for (int number : numbers) {
		leda::dic_item r = leda.lookup(std::rand() % ModCount);
		if (r) {
			ledaR += *leda.inf(r);
		}
	}
	bench.SwitchTest();

	std::srand(seed);
	bench.StartTest();
	for (int number : numbers) {
		int* r = impl.get(std::rand() % ModCount);
		if (r) {
			implR += *r;
		}
	}
	bench.StopTest();
	bench.PrintLast({ TestType::Get }, "Get " + std::to_string(N / 1000) + "k");

	if (ledaR != implR) {
		std::cout << "adding resulted in differences.\n";
	}
}

void delete_test(LedaTree& leda, ImplTree& impl, int N, int seed) {
	
	std::vector<int> numbers;
	for (int i = 0; i < N; ++i) {
		numbers.push_back(std::rand() % ModCount);
	}
	std::srand(seed);
	bench.StartTest();
	for (int number : numbers) {
		leda.undefine(number);
	}
	bench.SwitchTest();

	std::srand(seed);
	bench.StartTest();
	for (int number : numbers) {
		impl.remove(number);
	}
	bench.StopTest();
	bench.PrintLast({ TestType::Del }, "Del " + std::to_string(N / 1000) + "k");
}


int main() {
	LedaTree ledaDic(NodeSize / 2, NodeSize);
	ImplTree implDic;
	std::cout << "Memory size of Node: " << sizeof(ImplTree::TNode) << "\n";
#ifndef _DEBUG
	add_test	(ledaDic, implDic, 2000000, 0);
	delete_test (ledaDic, implDic, 1000000, 11);
	get_test	(ledaDic, implDic, 2000000, 1);
	add_test	(ledaDic, implDic, 2000000, 2);
	get_test	(ledaDic, implDic, 1500000, 3);
	delete_test	(ledaDic, implDic, 1500000, 4);
	get_test	(ledaDic, implDic, 1000000, 5);
	add_test	(ledaDic, implDic, 1000000, 6);
	delete_test	(ledaDic, implDic, 1000000, 7);
	get_test	(ledaDic, implDic, 1000000, 9);
	add_test	(ledaDic, implDic, 1000000, 8);
	delete_test (ledaDic, implDic, 1000000, 10);
	delete_test	(ledaDic, implDic, 1000000, 0);
	get_test	(ledaDic, implDic, 1000000, 0);
#else // in debug just run some basic stuff because all the tests take too much time
	add_test(ledaDic, implDic, 2000000, 0);
	delete_test(ledaDic, implDic, 1000000, 11);
	get_test(ledaDic, implDic, 2000000, 1);
#endif

	bench.Print();


	Timer.Print("Generic Timer");

	return 0;
}
#endif
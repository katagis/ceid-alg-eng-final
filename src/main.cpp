#ifndef _TESTS
#include <iostream>
#include "tree.h"
#include <unordered_set>

#include <LEDA/core/impl/ab_tree.h>
#include <LEDA/core/dictionary.h>



//constexpr int NodeSize = 338; // blocksize
constexpr int NodeSize = 64;

using LedaTree = leda::dictionary<int, int*, leda::ab_tree>;
using ImplTree = Tree<int, int, NodeSize>;

Benchmark bench;

constexpr int ModCount = 1000000;

void add_test(LedaTree& leda, ImplTree& impl, int N, int seed) {
	std::srand(seed);

	std::vector<int> numbers;
	std::vector<int*> ptr_numbers;
	numbers.reserve(N);
	ptr_numbers.reserve(N);

	for (int i = 0; i < N; ++i) {
		int num = std::rand() % ModCount;
		numbers.push_back(num);
		ptr_numbers.push_back(new int(num));
	}

	bench.StartTest();
	for (int i = 0; i < N; ++i) {
		leda.insert(numbers[i], ptr_numbers[i]);
	}
	bench.StopLeda();

	bench.StartTest();
	for (int i = 0; i < N; ++i) {
		impl.set(numbers[i], ptr_numbers[i]);
	}
	bench.StopImpl();
	bench.PrintLast({ TestType::Add }, "Add " + std::to_string(N / 1000) + "k");
}

void get_test(LedaTree& leda, ImplTree& impl, int N, int seed) {
	std::srand(seed);

	std::vector<int> numbers;
	for (int i = 0; i < N; ++i) {
		numbers.push_back(std::rand() % ModCount);
	}

	int ledaR = 0;
	int implR = 0;
	bench.StartTest();
	for (int number : numbers) {
		leda::dic_item r = leda.lookup(number);
		if (r) {
			ledaR += *leda.inf(r);
		}
	}
	bench.StopLeda();

	bench.StartTest();
	for (int number : numbers) {
		int* r = impl.get(number);
		if (r) {
			implR += *r;
		}
	}
	bench.StopImpl();
	bench.PrintLast({ TestType::Get }, "Get " + std::to_string(N / 1000) + "k");

	if (ledaR != implR) {
		std::cout << "adding resulted in differences.\n";
	}
}

void delete_test(LedaTree& leda, ImplTree& impl, int N, int seed) {
	std::srand(seed);
	std::vector<int> numbers;
	for (int i = 0; i < N; ++i) {
		numbers.push_back(std::rand() % ModCount);
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


int main() {
	LedaTree ledaDic(NodeSize / 2, NodeSize);
	ImplTree implDic;
	std::cout << "Memory size of Node: " << sizeof(ImplTree::TNode) << "\n";
#ifndef _DEBUG
	add_test	(ledaDic, implDic, 2000000, 0);
	delete_test (ledaDic, implDic, 1000000, 11);
	get_test	(ledaDic, implDic, 2000000, 1);
	add_test	(ledaDic, implDic, 2000000, 9874984);
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
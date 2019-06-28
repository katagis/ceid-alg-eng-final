#ifndef __TESTER_H_
#define __TESTER_H_

#include "tree.h"
#include "random_gen.h"
#include <LEDA/core/impl/ab_tree.h>
#include <LEDA/core/dictionary.h>

#define to_str(num) std::to_string((long long int)(num))

template<uint NodeSize>
struct Benchmark {
	typedef leda::dictionary<int, int*, leda::ab_tree> LedaTree;
	typedef Tree<int, int, NodeSize> ImplTree;
	typedef Iterator<int, int, NodeSize> TreeIterator;

	static void runTest(int AllowedMemoryGBs = 1) {
		LedaTree ledaDic(NodeSize / 2, NodeSize);
		ImplTree implDic;
		
		bench.Reset(NodeSize, sizeof(ImplTree::TNode));

		rd::setMax(2 * 1000 * 1000);

		std::vector<int*> ptrs;

		int seed = 98;

		add_test(ledaDic, implDic, 1000000, ++seed, ptrs);
		get_test(ledaDic, implDic, 1000000, ++seed);
		delete_ex(ledaDic, implDic, 500000, ++seed);

		add_test(ledaDic, implDic, 1000000, ++seed, ptrs);
		get_test(ledaDic, implDic, 1000000, ++seed);
		delete_test(ledaDic, implDic, 1000000, ++seed);
		add_test(ledaDic, implDic, 500000, ++seed, ptrs);
		get_test(ledaDic, implDic, 500000, ++seed);
		delete_test(ledaDic, implDic, 500000, ++seed);

		add_items(ledaDic, implDic, 5 * 1000 * 1000 * AllowedMemoryGBs);
		iterate_all(ledaDic, implDic);
		delete_ex(ledaDic, implDic, 500000, ++seed);


		bench.Print();

		for (int i = 0; i < ptrs.size(); ++i) {
			delete ptrs[i];
		}
		implDic.clear();
	}

	static void add_no_bench(LedaTree& leda, ImplTree& impl, int N, int seed, std::vector<int*>& outPtrs) {

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
			impl.set(numbers[i], outPtrs[i]);
		}
	}

	static void add_test(LedaTree& leda, ImplTree& impl, int N, int seed, std::vector<int*>& outPtrs) {
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
		bench.PrintLast(TestType::Add, "Add " + to_str(N / 1000) + "k");
	}

	static void get_test(LedaTree& leda, ImplTree& impl, int N, int seed) {
		rd::seed(seed);

		std::vector<int> numbers;
		for (int i = 0; i < N; ++i) {
			numbers.push_back(rd::get());
		}

		int ledaR = 0;
		int implR = 0;
		bench.StartTest();
		for (int i = 0; i < N; ++i) {
			leda::dic_item r = leda.lookup(numbers[i]);
			if (r) {
				ledaR ^= *leda.inf(r);
			}
		}
		bench.StopLeda();

		bench.StartTest();
		for (int i = 0; i < N; ++i) {
			int* num;
			if (impl.get(numbers[i], num)) {
				implR ^= *num;
			}
		}
		bench.StopImpl();
		bench.PrintLast(TestType::Get, "Get " + to_str(N / 1000) + "k");

		if (ledaR != implR) {
			std::cout << "comparision resulted in differences.\n";
		}
	}

	static void get_no_comp(LedaTree& leda, ImplTree& impl, int N, int seed) {
		rd::seed(seed);

		std::vector<int> numbers;
		for (int i = 0; i < N; ++i) {
			numbers.push_back(rd::get());
		}

		int ledaR = 0;
		int implR = 0;
		bench.StartTest();
		for (int i = 0; i < N; ++i) {
			leda::dic_item r = leda.lookup(numbers[i]);
			if (r) {
				++ledaR;
			}
		}
		bench.StopLeda();

		bench.StartTest();
		for (int i = 0; i < N; ++i) {
			if (impl.find(numbers[i]).exists) {
				++implR;
			}
		}
		bench.StopImpl();
		bench.PrintLast(TestType::Get, "Get NoCmp" + to_str(N / 1000) + "k");
		if (ledaR != implR) {
			std::cout << "comparision resulted in differences.\n";
		}
	}

	static void delete_test(LedaTree& leda, ImplTree& impl, int N, int seed) {
		rd::seed(seed);
		std::vector<int> numbers;
		for (int i = 0; i < N; ++i) {
			numbers.push_back(rd::get());
		}
		bench.StartTest();
		for (int i = 0; i < N; ++i) {
			leda.undefine(numbers[i]);
		}
		bench.StopLeda();

		bench.StartTest();
		for (int i = 0; i < N; ++i) {
			impl.remove(numbers[i]);
		}
		bench.StopImpl();
		bench.PrintLast(TestType::Del, "Del " + to_str(N / 1000) + "k");
	}

	static void delete_ex(LedaTree& leda, ImplTree& impl, int N, int seed) {
		rd::seed(seed);

		if (N > ((int)impl.size() * 3) / 4) {
			std::cerr << "N is too big at delete exact\n";
			return;
		}

		std::vector<int> numbers;
		for (int i = 0; i < N; ) {
			int number = rd::get();
			if (impl.find(number).exists) {
				numbers.push_back(rd::get());
				++i;
			}
		}

		bench.StartTest();
		for (int i = 0; i < N; ++i) {
			leda.undefine(numbers[i]);
		}
		bench.StopLeda();

		bench.StartTest();
		for (int i = 0; i < N; ++i) {
			impl.remove(numbers[i]);
		}
		bench.StopImpl();
		bench.PrintLast(TestType::Del, "Del Exact " + to_str(N / 1000) + "k");
	}

	static void iterate_all(LedaTree& leda, ImplTree& impl) {

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
		for (TreeIterator it = impl.first(); it.isValid(); ++it) {
			implR ^= it.key();
		}
		bench.StopImpl();

		if (ledaR != implR) {
			std::cout << "iteration resulted in differences.\n";
		}
		bench.PrintLast(TestType::Iterate, "Iter " + to_str(count / 1000000) + "m");
	}

	static void add_items(LedaTree& leda, ImplTree& impl, int count) {
		for (int i = 0; i < count; ++i) {
			leda.insert(i, NULL);
			impl.set(i, NULL);
		}
	}
};

#ifdef COMPILE_BENCHMARK
template<int Size>
void recursiveTest(int MemGBs) {
	recursiveTest<Size - 16>(MemGBs);
	Benchmark<Size>::runTest(MemGBs);
}

template<>
void recursiveTest<8>(int MemGBs) {
	Benchmark<8>::runTest(MemGBs);
}

template<>
void recursiveTest<16>(int MemGBs) {
	recursiveTest<8>(MemGBs);
	Benchmark<16>::runTest(MemGBs);
}

template<>
void recursiveTest<338>(int MemGBs) {
	recursiveTest<320>(MemGBs);
	Benchmark<338>::runTest(MemGBs);
}
#endif
void runFullBench(int MemGBs) {
#ifdef COMPILE_BENCHMARK
	recursiveTest<338>(MemGBs);
#else
	std::cout << "Full Benchmark has not been compiled. Define COMPILE_BENCHMARK when building or use 'make bench'\n";
#endif
}



#endif  // __TESTER_H_
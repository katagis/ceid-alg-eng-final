#include <iostream>
#include "node.h"
#include <unordered_set>

#ifndef _TESTS
int main2();

int main() {
	Tree<int, int, 5> tree;

	std::unordered_set<int> set;

	std::srand(0);

	for (int i = 0; i < 1000; ++i) {
		int* number = new int(std::rand() % 500);
		bool didInsert = set.insert(*number).second;
		std::cerr << "inserting: " << *number << "\n";
		


		if (didInsert != tree.set(*number, number)) {
			std::cerr << "error inserting: " << *number << "\n";
			getchar();
		}
		if (!tree.get(*number)) {
			std::cerr << "error finding: " << *number << "\n";
			getchar();
		}
		tree.validate_ptrs();
	}
	//tree.dot_print();

	for (auto number : set) {
		int* found = tree.get(number);
		if (!found || *found != number) {
			std::cerr << "error finding: " << number << "\n";
			getchar();
		}
	}
	tree.dot_print();

	main2();

	return 0;
}

int main2() {



	
	Tree<int, int, 100> tree;

	std::unordered_set<int> set;

	std::srand(0);

	for (int i = 0; i < 50000; ++i) {
		int* number = new int(std::rand() % 2500);
		bool didInsert = set.insert(*number).second;
		std::cerr << "inserting: " << *number << "\n";




		if (didInsert != tree.set(*number, number)) {
			std::cerr << "error inserting: " << *number << "\n";
			getchar();
		}
		if (!tree.get(*number)) {
			std::cerr << "error finding: " << *number << "\n";
			getchar();
		}
		tree.validate_ptrs();
	}
	//tree.dot_print();

	for (auto number : set) {
		int* found = tree.get(number);
		if (!found || *found != number) {
			std::cerr << "error finding: " << number << "\n";
			getchar();
		}
	}
	tree.dot_print();
	
	return 0;
}
#endif
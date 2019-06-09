#include <iostream>
#include "node.h"
#include <unordered_set>

#ifndef _TESTS
int main() {
	Tree<int, int, 5> tree;

	std::unordered_set<int> set;

	std::srand(0);

	for (int i = 0; i < 5000; ++i) {
		int* number = new int(std::rand() % 50000);
		bool didInsert = set.insert(*number).second;
		if (didInsert != tree.set(*number, number)) {
			std::cerr << "error inserting: " << *number << "\n";
			getchar();
		}

		tree.validate_ptrs();
		/*if (!didInsert) {
			delete number;
		}*/
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
}
#endif
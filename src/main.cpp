#include <iostream>
#include "node.h"
#include <unordered_set>

#ifndef _TESTS


#define INS(i) tree.set(i, new int(i)); set.insert(i)
void main2() {
	Tree<int, int, 4> tree;

	std::unordered_set<int> set;

	std::srand(0);



	for (int i = 0; i < 50; ++i) {
		int* number = new int(std::rand() % 10000);
		bool didInsert = set.insert(*number).second;
		tree.maybe_add(*number, number);
		if (!didInsert) {
			delete number;
		}
	}

	std::cerr << std::endl;


	for (int n = 0; n < 10000; ++n) {
		int i = std::rand() % 10000;
		bool setDidDelete = set.erase(i) > 0;
		if (!setDidDelete) {
			continue;
		}


		tree.dot_print();
		std::cerr << "removing: " << i << std::endl;


		if (!tree.remove(i)) {
			std::cerr << "failed to remove: " << i << std::endl;
			tree.dot_print();
			getchar();
		}

		tree.validate_ptrs();


		if (tree.get(i)) {
			std::cerr << "found after remove: " << i << std::endl;
			tree.dot_print();
			getchar();
		}

		for (auto z : set) {
			if (!tree.get(z)) {
				std::cerr << "failed to find: " << z << " after remove: " << i << std::endl;
				tree.dot_print();
				getchar();
			}
		}

	}
}


int main() {
	
	main2();

	//Tree<int, int, 5> tree;

	//std::unordered_set<int> set;

	//std::srand(0);

	//for (int i = 0; i < 100; ++i) {
	//	int* number = new int(std::rand() % 500);
	//	bool didInsert = set.insert(*number).second;
	//	std::cerr << "inserting: " << *number << "\n";
	//	


	//	if (didInsert != tree.set(*number, number)) {
	//		std::cerr << "error inserting: " << *number << "\n";
	//		getchar();
	//	}
	//	if (!tree.get(*number)) {
	//		std::cerr << "error finding: " << *number << "\n";
	//		getchar();
	//	}
	//	tree.validate_ptrs();
	//}

	//tree.dot_print();


	//for (auto number : set) {
	//	int* found = tree.get(number);
	//	if (!found || *found != number) {
	//		std::cerr << "error finding: " << number << "\n";
	//		getchar();
	//	}
	//}
	//tree.dot_print();

	//
	return 0;
}
#endif
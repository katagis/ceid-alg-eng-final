#include <iostream>
#include "node.h"
#include <unordered_set>

#ifndef _TESTS


#define INS(i) tree.set(i, new int(i))
void main2() {


	Tree<int, int, 3> tree;

	for (int i : {1, 7, 6, 3, 8, 9, 5, 11, 15, 16, 18, 20, 12, 14, 2, 4, 31, 30, 29, 28, 27, 24, 25, 26, 17, 22}) {
		INS(i);
	}
	for (int i : {1, 7, 6, 3, 8, 9, 5, 11, 15, 16, 18, 20, 12, 14, 2, 4, 31, 30, 29, 28, 27, 24, 25, 26, 17, 22}) {
		INS(i + 2);
	}
	std::cerr << std::endl << std::endl << std::endl;

	tree.dot_print();
	getchar();
	std::cerr << std::endl << std::endl << std::endl;

	for (int i : {8, 9, 16, 17, 18}) {
		tree.dot_print();
		std::cerr << "removing: " << i << std::endl;


		tree.validate_ptrs();
		if (!tree.remove(i)) {
			std::cerr << "failed to remove: " << i << std::endl;
			tree.dot_print();
			getchar();
		}
		if (tree.get(i)) {
			std::cerr << "foudn after remove: " << i << std::endl;
			tree.dot_print();
			getchar();
		}
	}
	tree.dot_print();









	//Tree<int, int, 3> tree;

	//for (int i : {1, 7, 6, 3, 8, 9, 5, 11, 15, 16, 18, 20, 12, 14, 2, 4, 19}) {
	//	INS(i);
	//}

	//tree.dot_print();

	//std::cerr << std::endl << std::endl << std::endl;

	//for (int i : {9, 6, 8, 1, 3, 11}) {
	//	tree.dot_print();
	//	std::cerr << "removing: " << i << std::endl;


	//	tree.validate_ptrs();
	//	if (!tree.remove(i)) {
	//		std::cerr << "failed to remove: " << i << std::endl;
	//		tree.dot_print();
	//		getchar();
	//	}
	//}
	//tree.dot_print();
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
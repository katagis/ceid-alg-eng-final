#include <iostream>
#include "node.h"

#ifndef _TESTS
int main() {
	Tree<int, int, 4> tree;
	
	std::srand(0);

	for (int i = 25; i > 0; --i) {
		int ins = std::rand() % 100;
		tree.set(ins, new int(i));
	}
	tree.dot_print();
}
#endif
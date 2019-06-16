#ifdef _TESTS
#define CATCH_CONFIG_MAIN
#include "node.h"

#include "catch.hpp"
#include <unordered_set>

//
//TEST_CASE( "set/get/del, size: 1", "[tree]" ) {
//	Tree<std::string, int, 20> tree;
//
//	REQUIRE(tree.size() == 0);
//
//	REQUIRE(tree.set("foo", new int(0)));
//	REQUIRE_FALSE(tree.set("foo", new int(1))); // return false if item already exists
//
//	REQUIRE(tree.size() == 1);
//
//	REQUIRE(tree.get("foo"));		 // value should exist
//	REQUIRE(*tree.get("foo") == 1); // value should be updated
//
//	REQUIRE_FALSE(tree.get("foo2"));
//
//	REQUIRE(tree.size() == 1);
//
//	REQUIRE_FALSE(tree.remove("foo2"));
//	REQUIRE(tree.size() == 1);
//
//	REQUIRE(tree.remove("foo"));	 // return true if something was removed
//	REQUIRE_FALSE(tree.get("foo")); // no item at such 
//		
//	REQUIRE(tree.size() == 0);
//}
//
//TEST_CASE("set/get/del/clear, size: 1000", "[tree]") {
//	Tree<int, int, 6> tree;
//
//	for (int i = 0; i < 1000; ++i) {
//		REQUIRE(tree.set(i, new int(0)));
//	}
//
//	REQUIRE(tree.size() == 1000);
//	for (int i = 0; i < 1000; ++i) {
//		REQUIRE_FALSE(tree.set(i, new int(i)));
//	}
//
//	REQUIRE(tree.size() == 1000);
//	for (int i = 0; i < 1000; ++i) {
//		REQUIRE(tree.get(i));
//		REQUIRE(*tree.get(i) == i);
//	}
//
//	/*tree.clear();
//	REQUIRE(tree.size() == 0);
//	REQUIRE(tree.empty());
//	*/
//}
//
//TEST_CASE("random insert even", "[tree]") {
//	Tree<int, int, 4> tree;
//
//	std::unordered_set<int> set;
//
//	std::srand(0);
//
//	for (int i = 0; i < 15000; ++i) {
//		int* number = new int(std::rand() % 20000);
//		bool didInsert = set.insert(*number).second;
//		REQUIRE(tree.maybe_add(*number, number) == didInsert);
//		if (!didInsert) {
//			delete number;
//		}
//	}
//
//	REQUIRE(set.size() == tree.size());
//	for (auto number : set) {
//		int* found = tree.get(number);
//		REQUIRE(found);
//		REQUIRE((found && *found == number));
//		// TODO: memory leak if test fails.
//		if (found) {
//			delete found;
//		}
//	}
//
//}
//
//TEST_CASE("random insert odd", "[tree]") {
//	Tree<int, int, 3> tree;
//
//	std::unordered_set<int> set;
//
//	std::srand(1);
//
//	for (int i = 0; i < 15000; ++i) {
//		int* number = new int(std::rand() % 20000);
//		bool didInsert = set.insert(*number).second;
//		REQUIRE(tree.maybe_add(*number, number) == didInsert);
//		if (!didInsert) {
//			delete number;
//		}
//	}
//
//	REQUIRE(set.size() == tree.size());
//	for (auto number : set) {
//		int* found = tree.get(number);
//		REQUIRE(found);
//		REQUIRE((found && *found == number));
//		// TODO: memory leak if test fails.
//		if (found) {
//			delete found;
//		}
//	}
//
//}

template<uint N>
void testAll(int seed) {
	Tree<int, int, N> tree;

	std::unordered_set<int> set;

	std::srand(seed);

	for (int i = 0; i < 20000; ++i) {
		int* number = new int(std::rand() % 10000);
		bool didInsert = set.insert(*number).second;
		REQUIRE(tree.maybe_add(*number, number) == didInsert);
		if (!didInsert) {
			delete number;
		}
	}

	REQUIRE(set.size() == tree.size());
	for (auto number : set) {
		int* found = tree.get(number);
		REQUIRE(found);
		REQUIRE((found && *found == number));
	}
	
	//tree.dot_print();

	for (int i = 0; i < 10000; ++i) {
		int number = std::rand() % 10000;
		bool setDidDelete = set.erase(number) > 0;
		if (setDidDelete) {
			std::cerr << "i is: " << i << " deleting: " << number << std::endl;

		}
		else {
			continue;
		}
		
		int* deleted = tree.removePop(number);

		bool treeDidDelete = deleted != nullptr;

		if (i > 2) {
			std::cerr << i << std::endl;
			tree.validate_ptrs();
			for (auto znumber : set) {
				int* found = tree.get(znumber);
				if (!found) {
					std::cerr << znumber << " failed @" << i << " after deleting: " << number << std::endl;
					//tree.dot_print();
					getchar();
					break;
				}
			}
		}


		REQUIRE(treeDidDelete == setDidDelete);
		delete deleted;
	}

	REQUIRE(set.size() == tree.size());
	for (auto number : set) {
		int* found = tree.get(number);
		REQUIRE(found);
		REQUIRE((found && *found == number));
	}

	
	std::srand(seed);
	// this will produce the initial sequence inserted so all elements should be deleted.
	for (int i = 0; i < 15000; ++i) {
		int number = std::rand() % 20000;
		bool didDelete = set.erase(number) > 0;
		int* deleted = tree.removePop(number);

		REQUIRE((deleted != nullptr) == didDelete);
		delete deleted;
	}
	
	//tree.dot_print();
}

TEST_CASE("testAll @4 / seed: 0", "[tree]") {
	testAll<4>(0);
}



#endif
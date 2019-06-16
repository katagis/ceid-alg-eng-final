#ifdef _TESTS
#define CATCH_CONFIG_MAIN
#include "node.h"

#include "catch.hpp"
#include <unordered_set>


TEST_CASE( "set/get/del, size: 1", "[tree]" ) {
	Tree<std::string, int, 20> tree;

	REQUIRE(tree.size() == 0);

	REQUIRE(tree.set("foo", new int(0)));
	REQUIRE_FALSE(tree.set("foo", new int(1))); // return false if item already exists

	REQUIRE(tree.size() == 1);

	REQUIRE(tree.get("foo"));		 // value should exist
	REQUIRE(*tree.get("foo") == 1); // value should be updated

	REQUIRE_FALSE(tree.get("foo2"));

	REQUIRE(tree.size() == 1);

	REQUIRE_FALSE(tree.remove("foo2"));
	REQUIRE(tree.size() == 1);

	REQUIRE(tree.remove("foo"));	 // return true if something was removed
	REQUIRE_FALSE(tree.get("foo")); // no item at such 
		
	REQUIRE(tree.size() == 0);
}

TEST_CASE("set/get/del/clear, size: 1000", "[tree]") {
	Tree<int, int, 6> tree;

	for (int i = 0; i < 1000; ++i) {
		REQUIRE(tree.set(i, new int(0)));
	}

	REQUIRE(tree.size() == 1000);
	for (int i = 0; i < 1000; ++i) {
		REQUIRE_FALSE(tree.set(i, new int(i)));
	}

	REQUIRE(tree.size() == 1000);
	for (int i = 0; i < 1000; ++i) {
		REQUIRE(tree.get(i));
		REQUIRE(*tree.get(i) == i);
	}

	/*tree.clear();
	REQUIRE(tree.size() == 0);
	REQUIRE(tree.empty());
	*/
}

TEST_CASE("random insert even", "[tree]") {
	Tree<int, int, 4> tree;

	std::unordered_set<int> set;

	std::srand(0);

	for (int i = 0; i < 15000; ++i) {
		int* number = new int(std::rand() % 20000);
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
		// TODO: memory leak if test fails.
		if (found) {
			delete found;
		}
	}

}

TEST_CASE("random insert odd", "[tree]") {
	Tree<int, int, 3> tree;

	std::unordered_set<int> set;

	std::srand(1);

	for (int i = 0; i < 15000; ++i) {
		int* number = new int(std::rand() % 20000);
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
		// TODO: memory leak if test fails.
		if (found) {
			delete found;
		}
	}

}

template<uint NodeSize, bool Verify, uint Size>
void testAll(int seed) {
	Tree<int, int, NodeSize> tree;

	std::unordered_set<int> set;

	std::srand(seed);

	for (int i = 0; i < Size; ++i) {
		int* number = new int(std::rand() % 15000);
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
	
	for (int n = 0; n < Size; ++n) {
		for (int i = 0; i < Size/5; ++i) {
			int* number = new int(std::rand() % 15000);
			bool didInsert = set.insert(*number).second;
			REQUIRE(tree.maybe_add(*number, number) == didInsert);
			if (!didInsert) {
				delete number;
			}
		}

		for (int i = 0; i < Size/3; ++i) {
			int number = std::rand() % 15000;
			bool setDidDelete = set.erase(number) > 0;
			if (!setDidDelete) {
				continue;
			}

			int* deleted = tree.removePop(number);

			bool treeDidDelete = deleted != nullptr;

			if constexpr (Verify) {
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
	}
	

	REQUIRE(set.size() == tree.size());
	for (auto number : set) {
		int* found = tree.get(number);
		REQUIRE(found);
		REQUIRE((found && *found == number));
	}

	for (int number = 0; number < 15000; ++number) {
		bool didDelete = set.erase(number) > 0;
		int* deleted = tree.removePop(number);

		REQUIRE((deleted != nullptr) == didDelete);
		delete deleted;
	}

	REQUIRE(set.size() == tree.size());
	REQUIRE(tree.size() == 0);
}

TEST_CASE("test No verify 3,4,5,6", "[tree]") {
	testAll<3, false, 5000>(3);
	testAll<4, false, 5000>(3);
	//testAll<5, false, 5000>(3);
	//testAll<6, false, 5000>(4);
}



#endif
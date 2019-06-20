#ifdef _TESTS
#define CATCH_CONFIG_MAIN
#include "tree.h"

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
void verifyIterator(Tree<int, int, NodeSize>& tree, std::unordered_set<int>& set) {
	REQUIRE(tree.size(), set.size());

	int treeKeyTotal = 0;
	int treePtrTotal = 0;
	int setTotal = 0;

	int elemsIterated = 0;

	int prevKey = -1;

	for (Iterator it = tree.first(); it.isValid(); it.next()) {
		elemsIterated++;
		treeKeyTotal ^= it.key();
		treePtrTotal ^= *it.value();
		REQUIRE(it.key() == *it.value());
		REQUIRE(prevKey < it.key());
		prevKey = it.key();
		it.next();
	}
	REQUIRE(treeKeyTotal == treePtrTotal);
	REQUIRE(elemsIterated == tree.size());

	for (auto el : set) {
		setTotal ^= el;
	}

	REQUIRE(treeKeyTotal == setTotal);
}

template<uint NodeSize, bool Verify, uint Size>
void testAll(int seed) {
	Tree<int, int, NodeSize> tree;

	std::unordered_set<int> set;

	std::srand(seed);

	for (int i = 0; i < Size; ++i) {
		int* number = new int(std::rand() % 8000);
		bool didInsert = set.insert(*number).second;
		REQUIRE(tree.maybe_add(*number, number) == didInsert);
		if (!didInsert) {
			delete number;
		}
	}

	REQUIRE(set.size() == tree.size());

	verifyIterator(tree, set);
	
	for (auto number : set) {
		int* found = tree.get(number);
		REQUIRE(found);
		REQUIRE((found && *found == number));
	}
	
	for (int n = 0; n < Size; ++n) {
		for (int i = 0; i < Size/5; ++i) {
			int* number = new int(std::rand() % 8000);
			bool didInsert = set.insert(*number).second;
			REQUIRE(tree.maybe_add(*number, number) == didInsert);
			if (!didInsert) {
				delete number;
			}
		}

		verifyIterator(tree, set);

		for (int i = 0; i < Size/3; ++i) {
			int number = std::rand() % 8000;
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
						getchar();
						break;
					}
				}
			}

			REQUIRE(treeDidDelete == setDidDelete);
			delete deleted;
		}
	}
	

	verifyIterator(tree, set);

	REQUIRE(set.size() == tree.size());
	for (auto number : set) {
		int* found = tree.get(number);
		REQUIRE(found);
		REQUIRE((found && *found == number));
	}

	for (int number = 0; number < 8000; ++number) {
		bool didDelete = set.erase(number) > 0;
		int* deleted = tree.removePop(number);

		REQUIRE((deleted != nullptr) == didDelete);
		delete deleted;
	}

	REQUIRE(set.size() == tree.size());
	REQUIRE(tree.size() == 0);
}

TEST_CASE("test No verify 3,4,5,6", "[tree]") {
	testAll<3, false, 1000>(1);
	testAll<4, false, 1000>(2);
	testAll<5, false, 1000>(3);
	testAll<6, false, 1000>(4);
}

#endif
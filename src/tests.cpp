#ifdef _TESTS
#define CATCH_CONFIG_MAIN
#include "tree.h"
#include "random_gen.h"
#include "catch.hpp"
#include <unordered_set>


TEST_CASE( "set/get/del, size: 1", "[tree]" ) {
	Tree<std::string, int, 20> tree;

	REQUIRE(tree.size() == 0);

	REQUIRE(tree.set("foo", new int(0)));
	REQUIRE_FALSE(tree.set("foo", new int(1))); // return false if item already exists

	REQUIRE(tree.size() == 1);

	REQUIRE(tree.find("foo").exists);		 // value should exist
	int* val;
	tree.get("foo", val);
	REQUIRE(*val == 1); // value should be updated

	REQUIRE_FALSE(tree.find("foo2").exists);

	REQUIRE(tree.size() == 1);

	REQUIRE_FALSE(tree.remove("foo2"));
	REQUIRE(tree.size() == 1);

	REQUIRE(tree.remove("foo"));	 // return true if something was removed
	REQUIRE_FALSE(tree.find("foo").exists); // no item at such 
		
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
		int* num;
		REQUIRE(tree.get(i, num));
		REQUIRE(*num == i);
	}

	tree.clear();
	REQUIRE(tree.size() == 0);
	REQUIRE(tree.empty());
}

TEST_CASE("random insert even", "[tree]") {
	Tree<int, int, 4> tree;

	std::unordered_set<int> set;

	rd::seed(0);
	rd::setMax(20000);

	for (int i = 0; i < 15000; ++i) {
		int* number = new int(rd::get() % 20000);
		bool didInsert = set.insert(*number).second;
		REQUIRE(tree.maybe_add(*number, number) == didInsert);
		if (!didInsert) {
			delete number;
		}
	}

	REQUIRE(set.size() == tree.size());
	for (auto number : set) {
		int* found;
		bool exists = tree.get(number, found);
		REQUIRE(exists);
		REQUIRE((exists && *found == number));
		if (found) {
			delete found;
		}
	}

}

TEST_CASE("random insert odd", "[tree]") {
	Tree<int, int, 3> tree;

	std::unordered_set<int> set;

	rd::seed(1);
	rd::setMax(20000);
	for (int i = 0; i < 15000; ++i) {
		int* number = new int(rd::get());
		bool didInsert = set.insert(*number).second;
		REQUIRE(tree.maybe_add(*number, number) == didInsert);
		if (!didInsert) {
			delete number;
		}
	}

	REQUIRE(set.size() == tree.size());
	for (auto number : set) {
		int* found;
		REQUIRE(tree.get(number, found));
		REQUIRE(*found == number);
		if (found) {
			delete found;
		}
	}

}

template<uint NodeSize>
void verifyIterator(Tree<int, int, NodeSize>& tree, std::unordered_set<int>& set) {
	REQUIRE(tree.size() == set.size());

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
	constexpr int MaxNum = 4000;
	Tree<int, int, NodeSize> tree;

	std::unordered_set<int> set;

	rd::seed(seed);
	rd::setMax(MaxNum);


	for (int i = 0; i < Size; ++i) {
		int* number = new int(rd::get());
		bool didInsert = set.insert(*number).second;
		REQUIRE(tree.maybe_add(*number, number) == didInsert);
		if (!didInsert) {
			delete number;
		}
	}

	REQUIRE(set.size() == tree.size());

	verifyIterator(tree, set);
	
	for (auto number : set) {
		int* found;
		REQUIRE(tree.get(number, found));
		REQUIRE(*found == number);
	}
	
	for (int n = 0; n < Size; ++n) {
		for (int i = 0; i < Size/5; ++i) {
			int* number = new int(rd::get());
			bool didInsert = set.insert(*number).second;
			REQUIRE(tree.maybe_add(*number, number) == didInsert);
			if (!didInsert) {
				delete number;
			}
		}

		verifyIterator(tree, set);

		for (int i = 0; i < Size/3; ++i) {
			int number = rd::get();
			bool setDidDelete = set.erase(number) > 0;
			if (!setDidDelete) {
				continue;
			}

			int* deleted;
			bool treeDidDelete = tree.removePop(number, deleted);

			if constexpr (Verify) {
				tree.checkIntegrity();
				for (auto znumber : set) {
					auto found = tree.find(znumber);
					if (!found.exists) {
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
		int* found;
		REQUIRE(tree.get(number, found));
		REQUIRE(*found == number);
	}

	for (int number = 0; number < MaxNum; ++number) {
		bool didDelete = set.erase(number) > 0;

		int* elem;
		bool del = tree.removePop(number, elem);

		REQUIRE(del == didDelete);
		if (del) {
			delete elem;
		}
	}

	REQUIRE(set.size() == tree.size());
	REQUIRE(tree.size() == 0);
}

TEST_CASE("test all 3,4,5,6", "[tree]") {
	testAll<3, true, 800>(1);
	testAll<4, true, 800>(2);
	testAll<5, true, 800>(3);
	testAll<6, true, 800>(4);
}

TEST_CASE("test iterator", "[tree]") {
	Tree<int, int, 4> tree;

	for (int i = 0; i < 100; ++i) {
		tree.set(i, new int(i));
	}

	REQUIRE(*tree.find(12).value() == 12);
	REQUIRE(tree.find(12).key() == 12);

	int num = 10;
	for (auto it = tree.find(10); it.key() < 50; ++it) {
		REQUIRE(num == it.key());
		num++;
	}
	REQUIRE(num == 50);
}


#endif
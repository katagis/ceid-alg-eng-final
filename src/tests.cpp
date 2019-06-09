#ifdef _TESTS
#define CATCH_CONFIG_MAIN
#include "node.h"

#include "catch.hpp"
#include <unordered_set>

/*
TEST_CASE( "set/get/del, size: 1", "[tree]" ) {
	Tree<std::string, int, 20> tree;



	SECTION("set & insert") {

		REQUIRE(tree.size() == 0);

		REQUIRE(tree.set("foo", new int(0)));
		REQUIRE_FALSE(tree.set("foo", new int(1))); // return false if item already exists

		REQUIRE(tree.size() == 1);
	}

	SECTION("get & query") {

		REQUIRE(tree.get("foo"));		 // value should exist
		REQUIRE(*tree.get("foo") == 1); // value should be updated

		REQUIRE_FALSE(tree.get("foo2"));

		REQUIRE(tree.size() == 1);
	}

	SECTION("remove") {
		REQUIRE_FALSE(tree.remove("foo2"));
		REQUIRE(tree.size() == 1);

		REQUIRE(tree.remove("foo"));	 // return true if something was removed
		REQUIRE_FALSE(tree.get("foo")); // no item at such 
		
		REQUIRE(tree.size() == 0);
	}
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

	tree.clear();
	REQUIRE(tree.size() == 0);
	REQUIRE(tree.empty());
}
*/
TEST_CASE("random insert", "[tree]") {
	Tree<int, int, 4> tree;

	std::unordered_set<int> set;

	std::srand(0);

	for (int i = 0; i < 15000; ++i) {
		int* number = new int(std::rand() % 50000);
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
	tree.dot_print();
}


#endif
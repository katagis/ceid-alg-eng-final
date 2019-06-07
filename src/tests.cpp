#ifdef _TESTS
#define CATCH_CONFIG_MAIN
#include "node.h"

#include "catch.hpp"


TEST_CASE( "set/get/del, size: 1", "[tree]" ) {
	Tree<std::string, int, 20> tree;

	SECTION("set & insert") {

		REQUIRE(tree.size() == 0);

		REQUIRE(tree.set(0, 0));
		REQUIRE_FALSE(tree.set("foo", 1)); // return false if item already exists

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

	SECTION("insert") {
		for (int i = 0; i < 1000; ++i) {
			REQUIRE(tree.set(i, 0));
		}
		REQUIRE(tree.size() == 1000);
	}

	SECTION("re-insert") {
		for (int i = 0; i < 1000; ++i) {
			REQUIRE_FALSE(tree.set(i, i));
		}
		REQUIRE(tree.size() == 1000);
	}

	SECTION("get") {
		for (int i = 0; i < 1000; ++i) {
			REQUIRE(tree.get(i));
			REQUIRE(*tree.get(i) == i);
		}
	}

	SECTION("clear") {
		tree.clear();
		REQUIRE(tree.size() == 0);
		REQUIRE(tree.empty());
	}
}

#endif
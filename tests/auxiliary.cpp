#include <catch.hpp>
#include <luwra.hpp>

#include <vector>
#include <stdexcept>

TEST_CASE("equal") {
	luwra::StateWrapper state;

	luwra::push(state, 1);
	luwra::push(state, 2);
	luwra::push(state, 1);

	SECTION("two indices") {
		REQUIRE(!luwra::equal(state, -1, -2));
		REQUIRE(!luwra::equal(state, -2, -1));

		REQUIRE(luwra::equal(state, -1, -3));
		REQUIRE(luwra::equal(state, -3, -1));
		REQUIRE(luwra::equal(state, -3, -3));
		REQUIRE(luwra::equal(state, -1, -1));
	}

	SECTION("iterable") {
		std::vector<int> indices {-1, -3, 1, 3};
		REQUIRE(luwra::equal(state, indices));
	}
}

TEST_CASE("setGlobal") {
	luwra::StateWrapper state;

	luwra::setGlobal(state, "test", 1338);

	REQUIRE(state.runString("return test") == LUA_OK);
	REQUIRE(luwra::read<int>(state, -1) == 1338);
}

TEST_CASE("getGlobal") {
	luwra::StateWrapper state;

	REQUIRE(state.runString("test = 1337") == LUA_OK);
	REQUIRE(luwra::getGlobal<int>(state, "test") == 1337);
}

TEST_CASE("setFields") {
	luwra::StateWrapper state;
	lua_newtable(state);

	SECTION("templateVersion") {
		luwra::setFields(
			state, -1,
			"test", 1338,
			123,    456
		);

		lua_setglobal(state, "test");

		REQUIRE(state.runString("return test.test, test[123]") == LUA_OK);
		REQUIRE(luwra::read<int>(state, -2) == 1338);
		REQUIRE(luwra::read<int>(state, -1) == 456);
	}

	SECTION("mapVersion") {
		luwra::setFields(
			state, -1,
			{
				{"test", 1338},
				{123,    456}
			}
		);

		lua_setglobal(state, "test");

		REQUIRE(state.runString("return test.test, test[123]") == LUA_OK);
		REQUIRE(luwra::read<int>(state, -2) == 1338);
		REQUIRE(luwra::read<int>(state, -1) == 456);
	}
}

TEST_CASE("getField") {
	luwra::StateWrapper state;

	REQUIRE(state.runString("return {hello = 123}") == LUA_OK);
	REQUIRE(luwra::getField<int>(state, -1, "hello") == 123);
}

static
void throw_runtime_error() {
	throw std::runtime_error("Halp");
}

TEST_CASE("StackUnwinding") {
	luwra::StateWrapper state;
	lua_atpanic(state, LUWRA_WRAP(throw_runtime_error));

	size_t counter = 0;

	try {
		struct PleaseDestructMe {
			size_t& r;

			PleaseDestructMe(size_t& c): r(c) {
				r++;
			}

			~PleaseDestructMe() {
				r--;
			}
		} pdm(counter);

		REQUIRE(counter == 1);

		// Force Lua error
		lua_pushnil(state);
		lua_call(state, 0, 0);
	} catch (...) {}

	REQUIRE(counter == 0);
}

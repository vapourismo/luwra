#include <catch.hpp>

#include <lua.hpp>
#include <luwra.hpp>

TEST_CASE("equal") {
	luwra::StateWrapper state;

	REQUIRE(luwra::push(state, 1) == 1);
	REQUIRE(luwra::push(state, 2) == 1);
	REQUIRE(luwra::push(state, 1) == 1);

	REQUIRE(!luwra::equal(state, -1, -2));
	REQUIRE(!luwra::equal(state, -2, -1));

	REQUIRE(luwra::equal(state, -1, -3));
	REQUIRE(luwra::equal(state, -3, -1));
	REQUIRE(luwra::equal(state, -3, -3));
	REQUIRE(luwra::equal(state, -1, -1));
}

TEST_CASE("setGlobal") {
	luwra::StateWrapper state;

	luwra::setGlobal(state, "test", 1338);

	REQUIRE(luaL_dostring(state, "return test") == LUA_OK);
	REQUIRE(luwra::read<int>(state, -1) == 1338);
}

TEST_CASE("getGlobal") {
	luwra::StateWrapper state;

	REQUIRE(luaL_dostring(state, "test = 1337") == LUA_OK);
	REQUIRE(luwra::getGlobal<int>(state, "test") == 1337);
}

TEST_CASE("setFields") {
	luwra::StateWrapper state;

	lua_newtable(state);
	luwra::setFields(
		state, -1,
		"test", 1338,
		123,    456
	);

	lua_setglobal(state, "test");

	REQUIRE(luaL_dostring(state, "return test.test, test[123]") == LUA_OK);
	REQUIRE(luwra::read<int>(state, -2) == 1338);
	REQUIRE(luwra::read<int>(state, -1) == 456);
}

TEST_CASE("getField") {
	luwra::StateWrapper state;

	REQUIRE(luaL_dostring(state, "return {hello = 123}") == LUA_OK);
	REQUIRE(luwra::getField<int>(state, -1, "hello") == 123);
}

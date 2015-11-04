#include <catch.hpp>

#include <lua.hpp>
#include <luwra.hpp>

static
int test_function_1(int a, int b) {
	return a - b;
}

static
int test_function_2(int a, int b, int c) {
	return a + b * c;
}

static
void test_function_3(int) {

}

TEST_CASE("stack_interaction") {
	lua_State* state = luaL_newstate();

	luwra::push(state, 1);
	luwra::push(state, 2);
	luwra::push(state, 4);

	// Redundant function
	luwra::apply(state, test_function_3);

	// Absolute index
	REQUIRE(luwra::apply(state, test_function_1) == -1);
	REQUIRE(luwra::apply(state, 1, test_function_1) == -1);
	REQUIRE(luwra::apply(state, test_function_2) == 9);
	REQUIRE(luwra::apply(state, 1, test_function_2) == 9);

	// Relative index
	REQUIRE(luwra::apply(state, -2, test_function_1) == -2);
	REQUIRE(luwra::apply(state, -3, test_function_1) == -1);
	REQUIRE(luwra::apply(state, -3, test_function_2) == 9);

	lua_close(state);
}

TEST_CASE("stack_register_global") {
	lua_State* state = luaL_newstate();

	luwra::register_global(state, "test", 1338);

	REQUIRE(luaL_dostring(state, "return test") == LUA_OK);
	REQUIRE(luwra::read<int>(state, -1) == 1338);

	lua_close(state);
}

TEST_CASE("stack_set_fields") {
	lua_State* state = luaL_newstate();

	lua_newtable(state);
	luwra::set_fields(
		state, -1,
		"test", 1338,
		123, 456
	);
	lua_setglobal(state, "test");

	REQUIRE(luaL_dostring(state, "return test.test, test[123]") == LUA_OK);
	REQUIRE(luwra::read<int>(state, -2) == 1338);
	REQUIRE(luwra::read<int>(state, -1) == 456);

	lua_close(state);
}

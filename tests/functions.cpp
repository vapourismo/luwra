#include <catch.hpp>
#include <luwra.hpp>

static
int noret_environment = 0;

static
void test_function_noret_noparams() {
	noret_environment++;
}

TEST_CASE("wrap_function_noret_noparams") {
	lua_State* state = luaL_newstate();

	// Setup environment
	noret_environment = 1337;

	// Wrap function
	// lua_CFunction cfun = luwra::wrap_function<void(), test_function_noret_noparams>;
	lua_CFunction cfun = LUWRA_WRAP_FUNCTION(test_function_noret_noparams);
	REQUIRE(cfun != nullptr);

	// Register function
	luwra::setGlobal(state, "test_function_noret_noparams", cfun);

	// Invoke function
	REQUIRE(luaL_dostring(state, "test_function_noret_noparams()") == 0);
	REQUIRE(lua_gettop(state) == 0);
	REQUIRE(noret_environment == 1338);

	lua_close(state);
}

static
void test_function_noret(int a, int b) {
	noret_environment = a + b;
}

TEST_CASE("wrap_function_noret") {
	lua_State* state = luaL_newstate();

	// Test function beforehand
	test_function_noret(13, 37);
	int req_environemt = noret_environment;

	// Wrap function
	// lua_CFunction cfun = luwra::wrap_function<void(int, int), test_function_noret>;
	lua_CFunction cfun = LUWRA_WRAP_FUNCTION(test_function_noret);
	REQUIRE(cfun != nullptr);

	// Register function
	luwra::setGlobal(state, "test_function_noret", cfun);

	// Invoke function
	REQUIRE(luaL_dostring(state, "test_function_noret(13, 37)") == 0);
	REQUIRE(lua_gettop(state) == 0);
	REQUIRE(noret_environment == req_environemt);

	lua_close(state);
}

static
int test_function_noparams() {
	return 13 * 37;
}

TEST_CASE("wrap_function_noparams") {
	lua_State* state = luaL_newstate();

	// Wrap function
	// lua_CFunction cfun = luwra::wrap_function<int(), test_function_noparams>;
	lua_CFunction cfun = LUWRA_WRAP_FUNCTION(test_function_noparams);
	REQUIRE(cfun != nullptr);

	// Register function
	luwra::setGlobal(state, "test_function_noparams", cfun);

	// Invoke function
	REQUIRE(luaL_dostring(state, "return test_function_noparams()") == 0);
	REQUIRE(lua_gettop(state) == 1);
	REQUIRE(luwra::Value<int>::read(state, -1) == test_function_noparams());

	lua_close(state);
}

static
int test_function(int a, int b) {
	return (a + b) * (a - b);
}

TEST_CASE("wrap_function") {
	lua_State* state = luaL_newstate();

	// Wrap function
	// lua_CFunction cfun = luwra::wrap_function<int(int, int), test_function>;
	lua_CFunction cfun = LUWRA_WRAP_FUNCTION(test_function);
	REQUIRE(cfun != nullptr);

	// Register function
	luwra::setGlobal(state, "test_function", cfun);

	// Invoke function
	REQUIRE(luaL_dostring(state, "return test_function(37, 13)") == 0);
	REQUIRE(lua_gettop(state) == 1);
	REQUIRE(luwra::Value<int>::read(state, -1) == test_function(37, 13));

	lua_close(state);
}

TEST_CASE("NativeFunction") {
	luwra::StateWrapper state;

	SECTION("with return value") {
		REQUIRE(luaL_dostring(state, "return function (x, y) return x + y end") == LUA_OK);

		auto fun = luwra::read<luwra::NativeFunction<int(int, int)>>(state, -1);
		REQUIRE(fun(13, 37) == 50);
	}

	SECTION("without return value") {
		REQUIRE(luaL_dostring(state, "return function (x, y) returnValue = x + y end") == LUA_OK);

		auto fun = luwra::read<luwra::NativeFunction<void(int, int)>>(state, -1);
		fun(13, 37);

		int returnValue = state["returnValue"];
		REQUIRE(returnValue == 50);
	}
}

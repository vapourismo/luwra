#include <catch.hpp>
#include <luwra.hpp>

static
int noret_environment = 0;

static
void test_function_noret_noparams() {
	noret_environment++;
}

static
void test_function_noret(int a, int b) {
	noret_environment = a + b;
}

static
int test_function_noparams() {
	return 13 * 37;
}

static
int test_function(int a, int b) {
	return (a + b) * (a - b);
}

TEST_CASE("FunctionWrapper") {
	luwra::StateWrapper state;

	SECTION("without return value, without parameters") {
		// Setup environment
		noret_environment = 1337;

		// Wrap function
		lua_CFunction cfun = LUWRA_WRAP(test_function_noret_noparams);
		REQUIRE(cfun != nullptr);

		// Register function
		state.set("test_function_noret_noparams", cfun);

		// Invoke function
		REQUIRE(state.runString("test_function_noret_noparams()") == LUA_OK);
		REQUIRE(lua_gettop(state) == 0);
		REQUIRE(noret_environment == 1338);
	}

	SECTION("without return value, with parameters") {
		// Test function beforehand
		test_function_noret(13, 37);
		int req_environemt = noret_environment;

		// Wrap function
		lua_CFunction cfun = LUWRA_WRAP(test_function_noret);
		REQUIRE(cfun != nullptr);

		// Register function
		state.set("test_function_noret", cfun);

		// Invoke function
		REQUIRE(state.runString("test_function_noret(13, 37)") == LUA_OK);
		REQUIRE(lua_gettop(state) == 0);
		REQUIRE(noret_environment == req_environemt);
	}

	SECTION("with return value, without parameters") {
		// Wrap function
		lua_CFunction cfun = LUWRA_WRAP(test_function_noparams);
		REQUIRE(cfun != nullptr);

		// Register function
		state.set("test_function_noparams", cfun);

		// Invoke function
		REQUIRE(state.runString("return test_function_noparams()") == LUA_OK);
		REQUIRE(lua_gettop(state) == 1);
		REQUIRE(luwra::read<int>(state, -1) == test_function_noparams());
	}

	SECTION("with return value, with parameters") {
		// Wrap function
		lua_CFunction cfun = LUWRA_WRAP(test_function);
		REQUIRE(cfun != nullptr);

		// Register function
		state.set("test_function", cfun);

		// Invoke function
		REQUIRE(state.runString("return test_function(37, 13)") == LUA_OK);
		REQUIRE(lua_gettop(state) == 1);
		REQUIRE(luwra::read<int>(state, -1) == test_function(37, 13));
	}
}

TEST_CASE("NativeFunction") {
	luwra::StateWrapper state;

	SECTION("with return value") {
		REQUIRE(state.runString("return function (x, y) return x + y end") == LUA_OK);

		auto fun = luwra::read<luwra::NativeFunction<int>>(state, -1);
		REQUIRE(fun(13, 37) == 50);
	}

	SECTION("without return value") {
		REQUIRE(state.runString("return function (x, y) returnValue = x + y end") == LUA_OK);

		auto fun = luwra::read<luwra::NativeFunction<void>>(state, -1);
		fun(13, 37);

		int returnValue = state.get<int>("returnValue");
		REQUIRE(returnValue == 50);
	}
}

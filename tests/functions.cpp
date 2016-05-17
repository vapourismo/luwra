#include <catch.hpp>
#include <luwra.hpp>

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

#include <catch.hpp>
#include <luwra.hpp>

TEST_CASE("NativeFunction<R>") {
	luwra::StateWrapper state;

	REQUIRE(state.runString("return function (x, y) return x + y end") == LUA_OK);

	auto fun = luwra::read<luwra::NativeFunction<int>>(state, -1);
	REQUIRE(fun(13, 37) == 50);
}

TEST_CASE("NativeFunction<void>") {
	luwra::StateWrapper state;

	REQUIRE(state.runString("return function (x, y) returnValue = x + y end") == LUA_OK);

	auto fun = luwra::read<luwra::NativeFunction<void>>(state, -1);
	fun(13, 37);

	int returnValue = state.get<int>("returnValue");
	REQUIRE(returnValue == 50);
}

TEST_CASE("function<R(A...)>") {
	luwra::StateWrapper state;

	REQUIRE(state.runString("return function (x, y) return x + y end") == LUA_OK);

	auto fun = luwra::read<std::function<int(int, int)>>(state, -1);
	REQUIRE(fun(13, 37) == 50);
}

TEST_CASE("function<void(A...)>") {
	luwra::StateWrapper state;

	REQUIRE(state.runString("return function (x, y) returnValue = x + y end") == LUA_OK);

	auto fun = luwra::read<std::function<void(int, int)>>(state, -1);
	fun(13, 37);

	int returnValue = state.get<int>("returnValue");
	REQUIRE(returnValue == 50);
}

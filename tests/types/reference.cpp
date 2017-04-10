#include <catch.hpp>
#include <luwra.hpp>

#include <iostream>

using namespace luwra;

TEST_CASE("RefHandle") {
	StateWrapper state;
	state.loadStandardLibrary();

	state["didCollect"] = false;

	REQUIRE(state.runString(
		"return setmetatable({}, {\n"
			"__gc = function () didCollect = true end"
		"})"
	) == LUA_OK);

	bool didCollect = state["didCollect"];
	REQUIRE(!didCollect);

	{
		RefLifecycle ref(state, -1);
		lua_pop(state, 1);

		didCollect = state["didCollect"];
		REQUIRE(!didCollect);

		lua_gc(state, LUA_GCCOLLECT, 0);

		didCollect = state["didCollect"];
		REQUIRE(!didCollect);
	}

	lua_gc(state, LUA_GCCOLLECT, 0);

	didCollect = state["didCollect"];
	REQUIRE(didCollect);
}

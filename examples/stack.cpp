#include <lua.hpp>
#include <luwra.hpp>

#include <iostream>

using namespace luwra;

static
lua_Integer sum3(lua_Integer a, lua_Integer b, lua_Integer c) {
	return a + b + c;
}

int main() {
	lua_State* state = luaL_newstate();
	luaL_openlibs(state);

	// Build stack
	lua_pushinteger(state, 13);
	lua_pushinteger(state, 37);
	lua_pushinteger(state, 42);

	// Each value can be retrieved individually.
	std::cout << "a = " << Value<lua_Integer>::read(state, 1) << std::endl;
	std::cout << "b = " << Value<lua_Integer>::read(state, 2) << std::endl;
	std::cout << "c = " << Value<lua_Integer>::read(state, 3) << std::endl;

	// ... which is a little cumbersome. Instead we might apply a fitting function to our stack.
	std::cout << "a + b + c = "
	          << apply(state, sum3)
	          << std::endl;

	lua_close(state);
	return 0;
}

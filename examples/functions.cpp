#include <lua.hpp>
#include <luwra.hpp>

#include <iostream>

using namespace luwra;

static
void my_function_1() {
	std::cout << "my_function_1()" << std::endl;
}

static
Integer my_function_2() {
	return 1338;
}

static
Integer my_function_3(Integer a, Integer b) {
	return a + b;
}

int main() {
	lua_State* state = luaL_newstate();

	// Register 'my_function_1'
	auto wrapped_1 = WrapFunction<void(), my_function_1>;
	lua_pushcfunction(state, wrapped_1);
	lua_setglobal(state, "my_function_1");

	// Register 'my_function_2'
	auto wrapped_2 = WrapFunction<Integer(), my_function_2>;
	lua_pushcfunction(state, wrapped_2);
	lua_setglobal(state, "my_function_2");

	// Register 'my_function_3'
	auto wrapped_3 = WrapFunction<Integer(Integer, Integer), my_function_3>;
	lua_pushcfunction(state, wrapped_3);
	lua_setglobal(state, "my_function_3");

	// Invoke the attached script
	luaL_loadfile(state, "functions.lua");
	lua_call(state, 0, LUA_MULTRET);

	lua_close(state);

	return 0;
}

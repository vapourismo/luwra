#include <lua.hpp>
#include <luwra.hpp>

#include <iostream>

using namespace luwra;

static
void my_function_1(float num, const char* str) {
	std::cout << "my_function_1(" << num << ", " << str << ")" << std::endl;
}

static
std::string my_function_2() {
	return "World";
}

static
int my_function_3(int a, int b) {
	return a + b;
}

int main() {
	lua_State* state = luaL_newstate();
	luaL_openlibs(state);

	// Register 'my_function_1'
	auto wrapped_1 = WrapFunction<void(float, const char*), my_function_1>;
	lua_pushcfunction(state, wrapped_1);
	lua_setglobal(state, "my_function_1");

	// Register 'my_function_2'
	auto wrapped_2 = WrapFunction<std::string(), my_function_2>;
	lua_pushcfunction(state, wrapped_2);
	lua_setglobal(state, "my_function_2");

	// Register 'my_function_3'
	auto wrapped_3 = WrapFunction<int(int, int), my_function_3>;
	lua_pushcfunction(state, wrapped_3);
	lua_setglobal(state, "my_function_3");

	// Invoke the attached script
	if (luaL_loadfile(state, "functions.lua") != 0 || lua_pcall(state, 0, LUA_MULTRET, 0) != 0) {
		const char* error_msg = lua_tostring(state, -1);
		std::cerr << "An error occured: " << error_msg << std::endl;

		lua_close(state);
		return 1;
	} else {
		lua_close(state);
		return 0;
	}
}

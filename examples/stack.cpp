#include <lua.hpp>
#include <luwra.hpp>

#include <iostream>

static
double sum3(int a, int b, double c) {
	return (a + b) * c;
}

int main() {
	lua_State* state = luaL_newstate();
	luaL_openlibs(state);

	// Build stack
	luwra::push(state, 13);
	luwra::push(state, 37);
	luwra::push(state, 42.2);

	// Each value can be retrieved individually.
	std::cout << "a = " << luwra::read<int>(state, 1) << std::endl;
	std::cout << "b = " << luwra::read<int>(state, 2) << std::endl;
	std::cout << "c = " << luwra::read<double>(state, 3) << std::endl;

	// ... which is a little cumbersome. Instead we might apply a fitting function to our stack.
	std::cout << "(a + b) * c = "
	          << luwra::apply(state, sum3)
	          << std::endl;

	lua_close(state);
	return 0;
}

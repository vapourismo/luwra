#include <lua.hpp>
#include <luwra.hpp>

#include <iostream>

using namespace luwra;

static
double sum3(int a, int b, double c) {
	return (a + b) * c;
}

int main() {
	lua_State* state = luaL_newstate();
	luaL_openlibs(state);

	// Build stack
	push(state, 13);
	push(state, 37);
	push(state, 42.2);

	// Each value can be retrieved individually.
	std::cout << "a = " << Value<int>::read(state, 1) << std::endl;
	std::cout << "b = " << Value<int>::read(state, 2) << std::endl;
	std::cout << "c = " << Value<double>::read(state, 3) << std::endl;

	// ... which is a little cumbersome. Instead we might apply a fitting function to our stack.
	std::cout << "(a + b) * c = "
	          << apply(state, sum3) // Equivalent to apply(state, 1, sum3) or apply(state, -3, sum3)
	          << std::endl;

	lua_close(state);
	return 0;
}

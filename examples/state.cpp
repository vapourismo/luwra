#include <lua.hpp>
#include <luwra.hpp>

#include <string>
#include <iostream>

using namespace luwra;

int main() {
	StateWrapper state;
	state.loadStandardLibrary();

	state["foo"] = 1337;

	int r = state.runString(
		"foo = foo + 1"
	);

	if (r != LUA_OK) {
		std::cerr << read<std::string>(state, -1) << std::endl;
		return 1;
	}

	int value = state["foo"];
	std::cout << value << std::endl;

	return 0;
}

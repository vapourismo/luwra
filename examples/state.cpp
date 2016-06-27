#include <luwra.hpp>

#include <string>
#include <iostream>

using namespace luwra;

int main() {
	StateWrapper state;
	state.loadStandardLibrary();

	state["foo"] = 1337;

	int r = state.runString(
		"print(foo)"
	);

	if (r != LUA_OK) {
		std::cerr << state.read<std::string>(-1) << std::endl;
		return 1;
	}

	return 0;
}

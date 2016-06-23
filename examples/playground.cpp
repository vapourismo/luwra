#include <luwra.hpp>
#include <iostream>
#include <list>

int main() {
	luwra::StateWrapper state;

	if (state.runString("return 'Hello World'") != LUA_OK) {
		std::cerr << luwra::read<std::string>(state, -1) << std::endl;
		return 1;
	}

	for (size_t i = 0; i < 50000000; i++) {
		std::string result = luwra::read<std::string>(state, -1);
	}

	return 0;
}

#include <iostream>
#include <luwra.hpp>

static int value = 1337;

int get() {
	return value;
}

void set(int v) {
	value = v;
}

int main() {
	luwra::StateWrapper state;

	state["get"] = LUWRA_WRAP(get);
	state["set"] = LUWRA_WRAP(set);

	if (state.runString("set(get() + 100)") != LUA_OK)
		std::cout << state.read<std::string>(-1) << std::endl;

	return 0;
}

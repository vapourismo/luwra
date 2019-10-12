#include <luwra.hpp>

#include <string>
#include <iostream>

using namespace luwra;

int main() {
	StateWrapper state;

	state["t1"] = MemberMap {};

	for (int i = 0; i < 10000; i++) {
		state["t1"]["value"] = i;
		int j = state["t1"]["value"];

		if (j != i) {
			return 1;
		}
	}

	state.runString("t2 = {a = 13, b = 37}");

	Table t2 = state["t2"];

	LuaType type = t2["a"];
	std::cout << (type == LuaType::Number) << std::endl;

	return 0;
}

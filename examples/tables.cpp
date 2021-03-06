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

	state.runString("t2 = {a = 13, b = 'Hello'}");

	state.runString("t2 = {a = 13, b = 'Hello'}");
	Table t2 = state["t2"];

	LuaType bt = t2["b"];
	if (bt == LuaType::String) {
		std::string b = t2["b"];
		std::cout << b << std::endl;
	}

	std::map<std::string, Reference> ret = state["t2"];
	for (auto pair: ret) {
		std::cout << pair.first << ": ";
		LuaType typ = pair.second;
		switch (typ) {
			case LuaType::Number:
				std::cout << pair.second.read<int>();
				break;

			case LuaType::String:
				std::cout << pair.second.read<std::string>();
				break;

			default:
				std::cout << "Unknown";
				break;

		}
		std::cout << std::endl;
	}

	return 0;
}

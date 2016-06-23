#include <luwra.hpp>
#include <iostream>
#include <list>

int main() {
	luwra::StateWrapper state;

	if (state.runString("foo = function (s) return s .. s; end") != LUA_OK) {
		std::cerr << luwra::read<std::string>(state, -1) << std::endl;
		return 1;
	}

	luwra::NativeFunction<std::string> foo = state["foo"];
	std::string input = "Hello World";
	std::string expected_output = input + input;

	for (size_t i = 0; i < 50000000; i++) {
		std::string result = foo(input);

		if (result != expected_output) {
			std::cerr << "Error" << std::endl;
			return 1;
		}
	}

	return 0;
}

#include <iostream>
#include <luwra.hpp>

struct Test {
	int value;

	void set(int v) {
		value = v;
	}

	int get() {
		return value;
	}
};

int main() {
	luwra::StateWrapper state;

	state.registerUserType<Test>({
		LUWRA_MEMBER(Test, set),
		LUWRA_MEMBER(Test, get)
	});

	state["test"] = Test {1337};

	if (state.runString("test:set(test:get() + 100)") != LUA_OK)
		std::cout << state.read<std::string>(-1) << std::endl;

	return 0;
}

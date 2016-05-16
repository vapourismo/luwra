#include <luwra.hpp>

#include <iostream>

static
void my_function_1(float num, const char* str) {
	std::cout << "my_function_1(" << num << ", " << str << ")" << std::endl;
}

static
std::string my_function_2() {
	return "World";
}

static
int my_function_3(int a, int b) {
	return a + b;
}

int main() {
	luwra::StateWrapper state;
	state.loadStandardLibrary();

	// Register 'my_function_1'
	auto wrapped_1 = LUWRA_WRAP(my_function_1);
	luwra::setGlobal(state, "my_function_1", wrapped_1);

	// Register 'my_function_2'
	auto wrapped_2 = LUWRA_WRAP(my_function_2);
	luwra::setGlobal(state, "my_function_2", wrapped_2);

	// Register 'my_function_3'
	auto wrapped_3 = LUWRA_WRAP(my_function_3);
	luwra::setGlobal(state, "my_function_3", wrapped_3);

	// Load Lua code
	int ret = state.runString(
		// Invoke 'my_function_1'
		"my_function_1(1337, 'Hello')\n"

		// Invoke 'my_function_2'
		"local result2 = my_function_2()\n"
		"print('my_function_2() = ' .. result2)\n"

		// Invoke 'my_function_3'
		"local result3 = my_function_3(13, 37)\n"
		"print('my_function_3(13, 37) = ' .. result3)\n"
	);

	// Invoke the attached script
	if (ret != LUA_OK) {
		std::cerr << "An error occured: " << lua_tostring(state, -1) << std::endl;
		return 1;
	}

	return 0;
}

#include <lua.hpp>
#include <luwra.hpp>

#include <iostream>

using namespace luwra;

// You may add custom specializations of luwra::Value in order to retrieve them from the stack.
template <>
struct Value<char> {
	/**
	 * Retrieve the value at position `n`.
	 */
	static inline
	char Read(State* state, int n) {
		auto str = Value<std::string>::Read(state, n);

		if (str.length() < 1) {
			luaL_argerror(state, n, "Given empty string instead of character");
		}

		return str[0];
	}

	/**
	 * Push the value onto the stack.
	 */
	static inline
	int Push(State* state, char val) {
		if (val == 0)
			return 0;

		lua_pushlstring(state, &val, 1);

		return 1;
	}
};

static
void read_chars(char a, char b) {
	std::cout << "Got '" << a << b << "'" << std::endl;
}

int main() {
	lua_State* state = luaL_newstate();
	luaL_openlibs(state);

	// Build stack
	Push(state, 'H');
	Push(state, 'i');

	// Apply function to stack values
	Apply(state, read_chars);

	lua_close(state);
	return 0;
}

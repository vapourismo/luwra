#include <luwra.hpp>

#include <iostream>

namespace luwra {
	// You may add custom specializations of luwra::Value in order to retrieve them from the stack.
	template <>
	struct Value<char> {
		/**
		 * Retrieve the value at position `n`.
		 */
		static inline
		char read(lua_State* state, int n) {
			auto str = Value<std::string>::read(state, n);

			if (str.length() < 1) {
				luaL_argerror(state, n, "Given empty string instead of character");
			}

			return str[0];
		}

		/**
		 * push the value onto the stack.
		 */
		static inline
		size_t push(lua_State* state, char val) {
			if (val == 0)
				return 0;

			lua_pushlstring(state, &val, 1);

			return 1;
		}
	};
}

static
void read_chars(char a, char b) {
	std::cout << "Got '" << a << b << "'" << std::endl;
}

int main() {
	luwra::StateWrapper state;
	state.loadStandardLibrary();

	// Build stack
	state.push('H');
	state.push('i');

	// apply function to stack values
	state.apply(1, read_chars);
	lua_pop(state, 2);

	// Build stack again
	state.push('Y', 'o');
	state.apply(1, read_chars);

	return 0;
}

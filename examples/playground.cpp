#include <iostream>
#include <stdexcept>
#include <luwra.hpp>

using namespace luwra;

struct D {
	D() {
		std::cout << "D()" << std::endl;
	}

	~D() {
		std::cout << "~D()" << std::endl;
	}
};

void atpanic(const Reference& ref) {
	throw std::runtime_error("Ellol");
}

int main() {
	StateWrapper state;
	lua_atpanic(state, LUWRA_WRAP(atpanic));

	{
		try {
			D d;
			lua_pushnil(state);
			lua_call(state, 0, 0);
		} catch (...) {

		}
	}

	return 0;
}

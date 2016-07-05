#include <luwra.hpp>
#include <iostream>
#include <string>

using namespace luwra;

struct StateBundle {
	State* state;
	bool close_state;

	inline
	StateBundle():
		state(luaL_newstate()),
		close_state(true)
	{}

	inline
	StateBundle(State* state):
		state(state),
		close_state(false)
	{}

	inline
	~StateBundle() {
		if (close_state) lua_close(state);
	}
};

struct StateWrapper2: StateBundle, Table {
	inline
	StateWrapper2():
		StateBundle(),
		Table({state, LUA_RIDX_GLOBALS, false})
	{}

	inline
	StateWrapper2(State* state):
		StateBundle(state),
		Table({state, LUA_RIDX_GLOBALS, false})
	{}
};

int main() {
	StateWrapper2 state(luaL_newstate());

	return 0;
}

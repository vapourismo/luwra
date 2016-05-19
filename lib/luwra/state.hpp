/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_STATEWRAPPER_H_
#define LUWRA_STATEWRAPPER_H_

#include "common.hpp"
#include "auxiliary.hpp"
#include "tables.hpp"

#include <string>
#include <utility>

LUWRA_NS_BEGIN

/**
 * Wrapper for a Lua state
 */
struct StateWrapper: Table {
	State* state;
	bool close_state;

	/**
	 * Operate on a foreign state instance.
	 */
	inline
	StateWrapper(State* state):
		Table(getGlobalsTable(state)),
		state(state),
		close_state(false)
	{}

	/**
	 * Create a new Lua state.
	 */
	inline
	StateWrapper():
		Table(getGlobalsTable(luaL_newstate())),
		state(ref.impl->state),
		close_state(true)
	{}

	inline
	~StateWrapper() {
		if (close_state)
			lua_close(state);
	}

	inline
	operator State*() {
		return state;
	}

	inline
	void loadStandardLibrary() {
		luaL_openlibs(state);
	}

	/**
	 * See [luwra::registerUserType](@ref luwra::registerUserType).
	 */
	template <typename T> inline
	void registerUserType(
		const char* ctor_name,
		const MemberMap& methods = MemberMap(),
		const MemberMap& meta_methods = MemberMap()
	) {
		::luwra::registerUserType<T>(state, ctor_name, methods, meta_methods);
	}

	/**
	 * Execute a piece of code.
	 */
	inline
	int runString(const char* code) {
		return luaL_dostring(state, code);
	}

	/**
	 * Execute a file.
	 */
	inline
	int runFile(const char* filepath) {
		return luaL_dofile(state, filepath);
	}
};

LUWRA_NS_END

#endif

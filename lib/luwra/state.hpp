/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_STATEWRAPPER_H_
#define LUWRA_STATEWRAPPER_H_

#include "common.hpp"
#include "auxiliary.hpp"

#include <string>
#include <utility>

LUWRA_NS_BEGIN

/**
 * Accessor for an entry in the global namespace
 */
struct GlobalAccessor {
	State* state;
	std::string key;

	inline
	GlobalAccessor(State* state, const std::string& key): state(state), key(key) {}

	/**
	 * Assign a new value.
	 */
	template <typename V> inline
	GlobalAccessor& set(V value) {
		setGlobal(state, key, value);
		return *this;
	}

	/**
	 * Shortcut for `set()`
	 */
	template <typename V> inline
	GlobalAccessor& operator =(V value) {
		return set<V>(value);
	}

	/**
	 * Retrieve the associated value.
	 */
	template <typename V> inline
	V get() {
		return getGlobal<V>(state, key);
	}

	/**
	 * Shortcut for `get()`
	 */
	template <typename V> inline
	operator V() {
		return get<V>();
	}
};

/**
 * Wrapper for a Lua state
 */
struct StateWrapper {
	State* state;
	bool close_state;

	/**
	 * Operate on a foreign state instance.
	 */
	inline
	StateWrapper(State* state): state(state), close_state(false) {}

	/**
	 * Create a new Lua state.
	 */
	inline
	StateWrapper(): state(luaL_newstate()), close_state(true) {}

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
	 * Retrieve a global value.
	 */
	template <typename V> inline
	V getGlobal(const std::string& key) const {
		return getGlobal<V>(state, key);
	}

	/**
	 * Assign a global value.
	 */
	template <typename V> inline
	void setGlobal(const std::string& key, V value) const {
		setGlobal(state, key, value);
	}

	/**
	 * Create an accessor to a global value.
	 */
	inline
	GlobalAccessor operator [](const std::string& key) const {
		return GlobalAccessor(state, key);
	}

	/**
	 * See [luwra::registerUserType](@ref luwra::registerUserType).
	 */
	template <typename T> inline
	void registerUserType(
		const std::string& ctor_name,
		const MemberMap& methods = MemberMap(),
		const MemberMap& meta_methods = MemberMap()
	) {
		registerUserType<T>(state, ctor_name, methods, meta_methods);
	}

	/**
	 * Execute a piece of code.
	 */
	inline
	int runString(const std::string& code) {
		return luaL_dostring(state, code.c_str());
	}

	/**
	 * Execute a file.
	 */
	inline
	int runFile(const std::string& filepath) {
		return luaL_dofile(state, filepath.c_str());
	}
};

LUWRA_NS_END

#endif

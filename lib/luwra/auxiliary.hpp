/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_AUXILIARY_H_
#define LUWRA_AUXILIARY_H_

#include "common.hpp"
#include "types.hpp"

LUWRA_NS_BEGIN

/**
 * Check if two values are equal.
 */
static inline
bool equal(State* state, int index1, int index2) {
#if LUA_VERSION_NUM <= 501
	return lua_equal(state, index1, index2);
#else
	return lua_compare(state, index1, index2, LUA_OPEQ);
#endif
}

/**
 * Register a value as a global.
 */
template <typename V> static inline
void setGlobal(State* state, const std::string& name, V value) {
	assert(1 == push(state, value));
	lua_setglobal(state, name.c_str());
}

/**
 * Retrieve a global value.
 */
template <typename V> static inline
V getGlobal(State* state, const std::string& name) {
	lua_getglobal(state, name.c_str());

	V instance = read<V>(state, -1);
	lua_pop(state, 1);

	return instance;
}

/**
 * Set multiple fields at once. Allows you to provide multiple key-value pairs.
 */
template <typename... R> static inline
void setFields(State* state, int index, R&&... args) {
	static_assert(sizeof...(R) % 2 == 0, "Field parameters must appear in pairs");
	internal::EntryPusher<R...>::push(state, index, std::forward<R>(args)...);
}

/**
 * Create a new table and set its fields.
 */
template <typename... R> static inline
void newTable(State* state, R&&... args) {
	lua_newtable(state);
	setFields(state, lua_gettop(state), std::forward<R>(args)...);
}

LUWRA_NS_END

#endif

/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_AUXILIARY_H_
#define LUWRA_AUXILIARY_H_

#include "common.hpp"
#include "types.hpp"

#include <vector>
#include <utility>

LUWRA_NS_BEGIN

/**
 * Check if two values are equal.
 * \param state  Lua state
 * \param index1 Index of left-hand side value
 * \param index2 Index of right-hand side value
 */
static inline
bool equal(State* state, int index1, int index2) {
#if LUA_VERSION_NUM <= 501
	return lua_equal(state, index1, index2) == 1;
#else
	return lua_compare(state, index1, index2, LUA_OPEQ) == 1;
#endif
}

/**
 * Set a registered metatable for the value on top of the stack.
 * \param state Lua state
 * \param name  Metatable name
 */
static inline
void setMetatable(State* state, const char* name) {
	luaL_newmetatable(state, name);
	lua_setmetatable(state, -2);
}

/**
 * Register a value as a global.
 * \param state Lua state
 * \param name  Global name
 * \param value Global value
 */
template <typename V> static inline
void setGlobal(State* state, const char* name, V&& value) {
	push(state, std::forward<V>(value));
	lua_setglobal(state, name);
}

/**
 * Retrieve a global value.
 * \param state Lua state
 * \param name  Global name
 * \returns Value associated with the given name
 */
template <typename V> static inline
V getGlobal(State* state, const char* name) {
	lua_getglobal(state, name);

	V instance = read<V>(state, -1);
	lua_pop(state, 1);

	return instance;
}

namespace internal {
	template <typename K, typename V, typename... R>
	struct EntryPusher {
		static inline
		void push(State* state, int index, K&& key, V&& value, R&&... rest) {
			EntryPusher<K, V>::push(state, index, std::forward<K>(key), std::forward<V>(value));
			EntryPusher<R...>::push(state, index, std::forward<R>(rest)...);
		}
	};

	template <typename K, typename V>
	struct EntryPusher<K, V> {
		static inline
		void push(State* state, int index, K&& key, V&& value) {
			luwra::push(state, std::forward<K>(key));
			luwra::push(state, std::forward<V>(value));
			lua_rawset(state, index < 0 ? index - 2 : index);
		}
	};
}

/**
 * Set multiple fields at once. Allows you to provide multiple key-value pairs.
 * \param state Lua state
 * \param index Table index
 * \param args  Key-value pairs
 */
template <typename... R> static inline
void setFields(State* state, int index, R&&... args) {
	static_assert(sizeof...(R) % 2 == 0, "Field parameters must appear in pairs");
	internal::EntryPusher<R...>::push(state, index, std::forward<R>(args)...);
}

/**
 * Map of members
 */
using MemberMap = std::map<Pushable, Pushable>;

/**
 * Apply key-value pairs to a table.
 * \param state  Lua state
 * \param index  Table index
 * \param fields Table fields
 */
static inline
void setFields(State* state, int index, const MemberMap& fields) {
	if (index < 0)
		index = lua_gettop(state) + (index + 1);

	for (const auto& entry: fields) {
		size_t pushedKeys = luwra::push(state, entry.first);
		if (pushedKeys > 1)
			lua_pop(state, static_cast<int>(pushedKeys - 1));

		size_t pushedValues = luwra::push(state, entry.second);
		if (pushedValues > 1)
			lua_pop(state, static_cast<int>(pushedValues - 1));

		lua_rawset(state, index);
	}
}

/**
 * Retrieve a field from a table.
 */
template <typename V, typename K> static inline
V getField(State* state, int index, K&& key) {
	if (index < 0)
		index = lua_gettop(state) + (index + 1);

	push(state, std::forward<K>(key));
	lua_rawget(state, index);

	V value = read<V>(state, -1);
	lua_pop(state, 1);

	return value;
}

LUWRA_NS_END

#endif

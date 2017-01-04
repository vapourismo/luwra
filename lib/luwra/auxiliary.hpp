/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_AUXILIARY_H_
#define LUWRA_AUXILIARY_H_

#include "common.hpp"
#include "types/pushable.hpp"
#include "stack.hpp"

#include <utility>
#include <map>

LUWRA_NS_BEGIN

/// Check if two values are equal.
///
/// \param state  Lua state
/// \param index1 Index of left-hand side value
/// \param index2 Index of right-hand side value
inline
bool equal(State* state, int index1, int index2) {
#if LUA_VERSION_NUM <= 501
	return lua_equal(state, index1, index2) == 1;
#else
	return lua_compare(state, index1, index2, LUA_OPEQ) == 1;
#endif
}

/// Check if 0 or more values are equal.
///
/// \param state   Lua state
/// \param indices Indices of values to compare
template <typename Iterable> inline
bool equal(State* state, const Iterable& indices) {
	auto it = indices.begin();
	auto end = indices.end();

	if (it == end)
		return true;

	int cmp_index = *it++;

	while (it != end) {
		int index = *it++;

		if (!equal(state, cmp_index, index))
			return false;

		cmp_index = index;
	}

	return true;
}

/// Set a registered metatable for the table on top of the stack.
///
/// \param state Lua state
/// \param name  Metatable name
inline
void setMetatable(State* state, const char* name) {
	luaL_newmetatable(state, name);
	lua_setmetatable(state, -2);
}

/// Register a value in the global namespace.
///
/// \param state Lua state
/// \param name  Global name
/// \param value Global value
template <typename Type> inline
void setGlobal(State* state, const char* name, Type&& value) {
	push(state, std::forward<Type>(value));
	lua_setglobal(state, name);
}

/// Retrieve a value from the global namespace.
///
/// \tparam Type Expected type of the value
///
/// \param state Lua state
/// \param name  Global name
template <typename Type> inline
Type getGlobal(State* state, const char* name) {
	lua_getglobal(state, name);

	Type instance = read<Type>(state, -1);
	lua_pop(state, 1);

	return instance;
}

/// Set the field of a table.
///
/// \param state Lua state
/// \param index Index of the table
/// \param key   Key
/// \param value %Value
template <typename Key, typename Type> inline
void setFields(State* state, int index, Key&& key, Type&& value) {
	push(state, std::forward<Key>(key));
	push(state, std::forward<Type>(value));
	lua_rawset(state, index < 0 ? index - 2 : index);
}

/// Set multiple fields at once. Allows you to provide multiple key-value pairs.
///
/// \param state  Lua state
/// \param index  Index of the table
/// \param key1   First key
/// \param value1 First value
/// \param key2   Second key
/// \param value2 Second value
/// \param rest   Rest of the key-value pairs
template <
	typename Key1,
	typename Type1,
	typename Key2,
	typename Type2,
	typename... Pairs
> inline
void setFields(
	State*     state,
	int        index,
	Key1&&     key1,
	Type1&&    value1,
	Key2&&     key2,
	Type2&&    value2,
	Pairs&&... rest
) {
	static_assert(sizeof...(Pairs) % 2 == 0, "Field parameters must appear in pairs");

	setFields(state, index, std::forward<Key1>(key1), std::forward<Type1>(value1));
	setFields(
		state,
		index,
		std::forward<Key2>(key2),
		std::forward<Type2>(value2),
		std::forward<Pairs>(rest)...
	);
}

/// Allows mixed-type map of members.
///
/// Example:
///
/// ```
///   MemberMap members {
///       {"foo", 13.37},
///       {1, "bar"},
///       {2, "baz"}
///   };
/// ```
using MemberMap = std::map<Pushable, Pushable>;

/// Apply key-value pairs to a table.
///
/// \param state  Lua state
/// \param index  %Table index
/// \param fields %Table fields
inline
void setFields(State* state, int index, const MemberMap& fields) {
	if (index < 0)
		index = lua_gettop(state) + (index + 1);

	for (const auto& entry: fields) {
		push(state, entry.first);
		push(state, entry.second);

		lua_rawset(state, index);
	}
}

/// Retrieve a field from a table.
///
/// \tparam Type Expected type of the value
///
/// \param state Lua state
/// \param index Index of the table
/// \param key   Key value
template <typename Type, typename Key> inline
Type getField(State* state, int index, Key&& key) {
	if (index < 0)
		index = lua_gettop(state) + (index + 1);

	push(state, std::forward<Key>(key));
	lua_rawget(state, index);

	Type value = read<Type>(state, -1);
	lua_pop(state, 1);

	return value;
}

LUWRA_NS_END

#endif

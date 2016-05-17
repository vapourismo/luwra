/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_TABLES_H_
#define LUWRA_TABLES_H_

#include "common.hpp"
#include "types.hpp"
#include "auxiliary.hpp"

LUWRA_NS_BEGIN

namespace internal {
	template <typename K>
	struct TableAccessor;
}

struct Table {
	Reference ref;

	Table(const Reference& ref):
		ref(ref)
	{}

	Table(State* state, int index):
		ref(state, index, true)
	{
		luaL_checktype(state, index, LUA_TTABLE);
	}

	template <typename K> inline
	internal::TableAccessor<K> access(K&& key);

	template <typename K> inline
	internal::TableAccessor<K> operator [](K&& key) {
		return access<K>(std::forward<K>(key));
	}

	inline
	void update(const FieldVector& fields) {
		State* state = ref.impl->state;

		push(state, ref);
		setFields(state, -1, fields);

		lua_pop(state, 1);
	}

	template <typename K> inline
	bool has(K&& key) {
		State* state = ref.impl->state;

		push(state, ref);

		size_t pushedKeys = push(state, std::forward<K>(key));
		if (pushedKeys > 1)
			lua_pop(state, static_cast<int>(pushedKeys - 1));

		lua_rawget(state, -2);
		bool isNil = lua_isnil(state, -1);

		lua_pop(state, 2);
		return !isNil;
	}

	template <typename V, typename K> inline
	void set(K&& key, V&& value) {
		State* state = ref.impl->state;
		push(state, ref);

		size_t pushedKeys = push(state, std::forward<K>(key));
		if (pushedKeys > 1)
			lua_pop(state, static_cast<int>(pushedKeys - 1));

		size_t pushedValues = push(state, std::forward<V>(value));
		if (pushedValues > 1)
			lua_pop(state, static_cast<int>(pushedValues - 1));

		lua_rawset(state, -3);
		lua_pop(state, 1);
	}

	template <typename V, typename K> inline
	V get(K&& key) {
		State* state = ref.impl->state;

		push(state, ref);

		size_t pushedKeys = push(state, std::forward<K>(key));
		if (pushedKeys > 1)
			lua_pop(state, static_cast<int>(pushedKeys - 1));

		lua_rawget(state, -2);
		V ret = read<V>(state, -1);

		lua_pop(state, 2);
		return ret;
	}
};

namespace internal {
	template <typename K>
	struct TableAccessor {
		Table parent;
		K key;

		template <typename V> inline
		V read() {
			return parent.get<V>(key);
		}

		template <typename V> inline
		operator V() {
			return parent.get<V>(key);
		}

		template <typename V> inline
		TableAccessor<K>& write(V&& value) {
			parent.set(key, std::forward<V>(value));
			return *this;
		}

		template <typename V> inline
		TableAccessor<K>& operator =(V&& value) {
			parent.set(key, std::forward<V>(value));
			return *this;
		}

		template <typename T> inline
		TableAccessor<T> access(T&& subkey) {
			return {read<Table>(), std::forward<T>(subkey)};
		}

		template <typename T> inline
		TableAccessor<T> operator [](T&& subkey) {
			return {read<Table>(), std::forward<T>(subkey)};
		}
	};
}

/**
 * Table
 */
template <typename K> inline
internal::TableAccessor<K> Table::access(K&& key) {
	return {*this, std::forward<K>(key)};
}

/**
 * See [Table](@ref Table).
 */
template <>
struct Value<Table> {
	static inline
	Table read(State* state, int index) {
		return {state, index};
	}

	static inline
	size_t push(State* state, const Table& value) {
		return value.ref.impl->push(state);
	}
};

/**
 * Retrieve the table containing all global values.
 * \param state Lua state
 * \returns Reference to the globals table.
 */
static inline
Table getGlobalsTable(State* state) {
#if LUA_VERSION_NUM <= 501
	return {{state, internal::referenceValue(state, LUA_GLOBALSINDEX), false}};
#else
	return {{state, LUA_RIDX_GLOBALS, false}};
#endif
}

LUWRA_NS_END

#endif

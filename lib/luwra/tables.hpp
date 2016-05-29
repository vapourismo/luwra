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
	// This represents a "path" which will be resolved lazily. It is useful for chained table
	// access. An access like 'table.field1.field2' would be represented similiar to
	// `Path<Path<Table, std::string>, std::string> table {{table, "field1"}, "field"}`.
	template <typename P, typename K>
	struct Path {
		P parent;
		K key;

		// Read the value to which this path points to.
		template <typename V> inline
		V read(State* state) const {
			luwra::push(state, *this);

			V value = luwra::read<V>(state, -1);

			lua_pop(state, 1);
			return value;
		}

		// Change the value to which this path points to.
		template <typename V> inline
		void write(State* state, V&& value) const {
			size_t pushedParents = luwra::push(state, parent);
			if (pushedParents > 1)
				lua_pop(state, static_cast<int>(pushedParents - 1));

			size_t pushedKeys = luwra::push(state, key);
			if (pushedKeys > 1)
				lua_pop(state, static_cast<int>(pushedKeys - 1));

			size_t pushedValues = luwra::push(state, std::forward<V>(value));
			if (pushedValues > 1)
				lua_pop(state, static_cast<int>(pushedValues - 1));

			lua_rawset(state, -3);
			lua_pop(state, 1);
		}
	};
}

template <typename P, typename K>
struct Value<internal::Path<P, K>> {
	// Push the value to which the path points onto the stack.
	static inline
	size_t push(State* state, const internal::Path<P, K>& accessor) {
		size_t pushedParents = luwra::push(state, accessor.parent);
		if (pushedParents > 1)
			lua_pop(state, static_cast<int>(pushedParents - 1));

		size_t pushedKeys = luwra::push(state, accessor.key);
		if (pushedKeys > 1)
			lua_pop(state, static_cast<int>(pushedKeys - 1));

		lua_rawget(state, -2);
		lua_remove(state, -2);

		return 1;
	}
};

struct Table;

namespace internal {
	template <typename A>
	class TableAccessor {
	public:
		State* state;
		A accessor;

	private:
		TableAccessor(const TableAccessor&) = default;
		TableAccessor(TableAccessor&&) = default;

		TableAccessor& operator =(const TableAccessor&) = delete;
		TableAccessor& operator =(TableAccessor&&) = delete;

		friend struct luwra::Table;

	public:
		template <typename V> inline
		V read() const && {
			return accessor.template read<V>(state);
		}

		template <typename V> inline
		operator V() const && {
			return accessor.template read<V>(state);
		}

		template <typename V> inline
		const TableAccessor&& write(V&& value) const && {
			accessor.write(state, std::forward<V>(value));
			return std::move(*this);
		}

		template <typename V> inline
		const TableAccessor&& operator =(V&& value) const && {
			accessor.write(state, std::forward<V>(value));
			return std::move(*this);
		}

		template <typename K> inline
		TableAccessor<Path<A, K>> access(K&& subkey) const && {
			return TableAccessor<Path<A, K>> {
				state,
				Path<A, K> {
					accessor,
					std::forward<K>(subkey)
				}
			};
		}

		template <typename K> inline
		TableAccessor<Path<A, K>> operator [](K&& subkey) const && {
			return {state, {accessor, std::forward<K>(subkey)}};
		}
	};
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

	Table(State* state):
		Table(state, (lua_newtable(state), -1))
	{}

	Table(State* state, const MemberMap& fields):
		Table(state, (luwra::push(state, fields), -1))
	{}

	template <typename K> inline
	internal::TableAccessor<internal::Path<const Reference&, K>> access(K&& key) const {
		return internal::TableAccessor<internal::Path<const Reference&, K>> {
			ref.impl->state,
			internal::Path<const Reference&, K> {
				ref,
				std::forward<K>(key)
			}
		};
	}

	template <typename K> inline
	internal::TableAccessor<internal::Path<const Reference&, K>> operator [](K&& key) const {
		return internal::TableAccessor<internal::Path<const Reference&, K>> {
			ref.impl->state,
			internal::Path<const Reference&, K> {
				ref,
				std::forward<K>(key)
			}
		};
	}

	inline
	void update(const MemberMap& fields) const {
		State* state = ref.impl->state;

		push(state, ref);
		setFields(state, -1, fields);

		lua_pop(state, 1);
	}

	template <typename K> inline
	bool has(K&& key) const {
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
	void set(K&& key, V&& value) const {
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
	V get(K&& key) const {
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

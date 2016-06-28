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
	// access. An access like 'table.field1.field2' would be represented similar to
	// `Path<Path<Table, std::string>, std::string> table {{table, "field1"}, "field"}`.
	template <typename Parent, typename Key>
	struct Path {
		Parent parent;
		Key key;

		// Read the value to which this path points to.
		template <typename V> inline
		V read(State* state) const {
			push(state, *this);

			V value = luwra::read<V>(state, -1);

			lua_pop(state, 1);
			return value;
		}

		// Change the value to which this path points to.
		template <typename V> inline
		void write(State* state, V&& value) const {
			push(state, parent);
			push(state, key);
			push(state, std::forward<V>(value));

			lua_rawset(state, -3);
			lua_pop(state, 1);
		}
	};
}

template <typename Parent, typename Key>
struct Value<internal::Path<Parent, Key>> {
	// Push the value to which the path points onto the stack.
	static inline
	size_t push(State* state, const internal::Path<Parent, Key>& accessor) {
		luwra::push(state, accessor.parent);
		luwra::push(state, accessor.key);

		lua_rawget(state, -2);
		lua_remove(state, -2);

		return 1;
	}
};

namespace internal {
	template <typename Accessor>
	struct TableAccessor {
		State* state;
		Accessor accessor;

		template <typename Type> inline
		Type read() const && {
			return accessor.template read<Type>(state);
		}

		template <typename Type> inline
		operator Type() const && {
			return accessor.template read<Type>(state);
		}

		template <typename Type> inline
		const TableAccessor&& write(Type&& value) const && {
			accessor.write(state, std::forward<Type>(value));
			return std::move(*this);
		}

		template <typename Type> inline
		const TableAccessor&& operator =(Type&& value) const && {
			accessor.write(state, std::forward<Type>(value));
			return std::move(*this);
		}

		template <typename Key> inline
		TableAccessor<Path<Accessor, Key>> access(Key&& subkey) const && {
			return TableAccessor<Path<Accessor, Key>> {
				state,
				Path<Accessor, Key> {
					accessor,
					std::forward<Key>(subkey)
				}
			};
		}

		template <typename Key> inline
		TableAccessor<Path<Accessor, Key>> operator [](Key&& subkey) const && {
			return TableAccessor<Path<Accessor, Key>> {
				state,
				Path<Accessor, Key> {
					accessor,
					std::forward<Key>(subkey)
				}
			};
		}
	};
}

template <typename Accessor>
struct Value<internal::TableAccessor<Accessor>> {
	static inline
	size_t push(State* state, internal::TableAccessor<Accessor>& ta) {
		return luwra::push(state, ta.accessor);
	}
};

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
	{
		lua_pop(state, 1);
	}

	Table(State* state, const MemberMap& fields):
		Table(state, (luwra::push(state, fields), -1))
	{
		lua_pop(state, 1);
	}

	template <typename Key> inline
	internal::TableAccessor<internal::Path<const Reference&, Key>> access(Key&& key) const {
		return internal::TableAccessor<internal::Path<const Reference&, Key>> {
			ref.impl->state,
			internal::Path<const Reference&, Key> {
				ref,
				std::forward<Key>(key)
			}
		};
	}

	template <typename Key> inline
	internal::TableAccessor<internal::Path<const Reference&, Key>> operator [](Key&& key) const {
		return internal::TableAccessor<internal::Path<const Reference&, Key>> {
			ref.impl->state,
			internal::Path<const Reference&, Key> {
				ref,
				std::forward<Key>(key)
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

	template <typename Key> inline
	bool has(Key&& key) const {
		State* state = ref.impl->state;

		push(state, ref);
		push(state, std::forward<Key>(key));

		lua_rawget(state, -2);
		bool isNil = lua_isnil(state, -1);

		lua_pop(state, 2);
		return !isNil;
	}

	template <typename Type, typename Key> inline
	void set(Key&& key, Type&& value) const {
		State* state = ref.impl->state;
		push(state, ref);
		push(state, std::forward<Key>(key));
		push(state, std::forward<Type>(value));

		lua_rawset(state, -3);
		lua_pop(state, 1);
	}

	template <typename Type, typename Key> inline
	Type get(Key&& key) const {
		State* state = ref.impl->state;

		push(state, ref);
		push(state, std::forward<Key>(key));

		lua_rawget(state, -2);
		Type ret = read<Type>(state, -1);

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

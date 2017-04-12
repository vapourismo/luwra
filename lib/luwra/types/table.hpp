/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_TYPES_TABLE_H_
#define LUWRA_TYPES_TABLE_H_

#include "../common.hpp"
#include "../auxiliary.hpp"
#include "../stack.hpp"
#include "reference.hpp"

#include <utility>

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
			push(state, parent);
			push(state, key);
			lua_rawget(state, -2);

			V value = luwra::read<V>(state, -1);

			lua_pop(state, 2);
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
	static inline
	void push(State* state, const internal::Path<Parent, Key>& accessor) {
		luwra::push(state, accessor.parent);
		luwra::push(state, accessor.key);

		lua_rawget(state, -2);
		lua_remove(state, -2);
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
		operator Type&() const & {
			return accessor.template read<Type&>(state);
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
		const TableAccessor<Path<Accessor, Key>> access(Key&& subkey) const && {
			return TableAccessor<Path<Accessor, Key>> {
				state,
				Path<Accessor, Key> {
					accessor,
					std::forward<Key>(subkey)
				}
			};
		}

		template <typename Key> inline
		const TableAccessor<Path<Accessor, Key>> operator [](Key&& subkey) const && {
			return TableAccessor<Path<Accessor, Key>> {
				state,
				Path<Accessor, Key> {
					accessor,
					std::forward<Key>(subkey)
				}
			};
		}
	};

	template <typename Parent, typename Key>
	using TableAccessorPath = TableAccessor<Path<Parent, Key>>;
}

template <typename Accessor>
struct Value<internal::TableAccessor<Accessor>> {
	static inline
	void push(State* state, const internal::TableAccessor<Accessor>& ta) {
		luwra::push(state, ta.accessor);
	}
};

/// Allows you to inspect and modify Lua tables.
struct Table {
	Reference ref;

	/// Create from reference.
	Table(const Reference& ref):
		ref(ref)
	{}

	/// Create from table on the stack. This will retain a reference to the table.
	Table(State* state, int index):
		ref(state, index)
	{
		luaL_checktype(state, index, LUA_TTABLE);
	}

	/// Create a new table.
	Table(State* state):
		ref((lua_newtable(state), state))
	{}

	/// Create a new table with the given fields.
	Table(State* state, const MemberMap& fields):
		ref((luwra::push(state, fields), state))
	{}

	/// Identical to @ref operator[].
	template <typename Key> inline
	const internal::TableAccessorPath<const Reference&, Key> access(Key&& key) const {
		return operator [](std::forward<Key>(key));
	}

	/// Create an accessor to a field of the table.
	///
	/// Example:
	///
	/// ```
	///   // Retrieve the value of a field.
	///   int value = table["fieldName"];
	///
	///   // Update the value of a field.
	///   table["fieldName"] = 13.37;
	///
	///   // Accessor nesting is also possible, assuming the field 'fieldName' is a table aswell.
	///   table["fieldName"]["nestedFieldName"] = 1337;
	///   int value = table["fieldName"]["nestedFieldName"];
	/// ```
	template <typename Key> inline
	const internal::TableAccessorPath<const Reference&, Key> operator [](Key&& key) const {
		return internal::TableAccessorPath<const Reference&, Key> {
			ref.life->state,
			internal::Path<const Reference&, Key> {
				ref,
				std::forward<Key>(key)
			}
		};
	}

	/// Update the table using the given map of members.
	inline
	void update(const MemberMap& fields) const {
		State* state = ref.life->state;

		push(state, ref);
		setFields(state, -1, fields);

		lua_pop(state, 1);
	}

	/// Check if the value associated with a key is not `nil`.
	template <typename Key> inline
	bool has(Key&& key) const {
		State* state = ref.life->state;

		push(state, ref);
		push(state, std::forward<Key>(key));

		lua_rawget(state, -2);
		bool isNil = lua_isnil(state, -1);

		lua_pop(state, 2);
		return !isNil;
	}

	/// Update a field.
	template <typename Type, typename Key> inline
	void set(Key&& key, Type&& value) const {
		State* state = ref.life->state;
		push(state, ref);
		push(state, std::forward<Key>(key));
		push(state, std::forward<Type>(value));

		lua_rawset(state, -3);
		lua_pop(state, 1);
	}

	/// Retrieve the value of a field.
	template <typename Type, typename Key> inline
	Type get(Key&& key) const {
		State* state = ref.life->state;

		push(state, ref);
		push(state, std::forward<Key>(key));

		lua_rawget(state, -2);
		Type ret = read<Type>(state, -1);

		lua_pop(state, 2);
		return ret;
	}
};

/// Enables reading/pushing tables
template <>
struct Value<Table> {
	static inline
	Table read(State* state, int index) {
		return {state, index};
	}

	static inline
	void push(State* state, const Table& value) {
		luwra::push(state, value.ref);
	}
};

LUWRA_NS_END

#endif

/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_TYPES_STL_H_
#define LUWRA_TYPES_STL_H_

#include "../common.hpp"
#include "../values.hpp"
#include "../stack.hpp"
#include "../internal/indexsequence.hpp"

#include <utility>
#include <tuple>
#include <vector>
#include <list>
#include <map>

LUWRA_NS_BEGIN

/// Enables pushing for `std::vector` assuming `Type` is also pushable
template <typename Type>
struct Value<std::vector<Type>> {
	static inline
	void push(State* state, const std::vector<Type>& vec) {
		lua_createtable(state, static_cast<int>(vec.size()), 0);

		int size = static_cast<int>(vec.size());
		for (int i = 0; i < size; i++) {
			luwra::push(state, vec[i]);
			lua_rawseti(state, -2, i + 1);
		}
	}
};

/// Enables pushing for `std::list` assuming `Type` is pushable
template <typename Type>
struct Value<std::list<Type>> {
	static inline
	void push(State* state, const std::list<Type>& lst) {
		lua_createtable(state, static_cast<int>(lst.size()), 0);

		int i = 0;
		for (const Type& item: lst) {
			luwra::push(state, item);
			lua_rawseti(state, -2, ++i);
		}
	}
};

/// Enables pushing and reading for `std::map` assuming `Key` and `Type` are pushable or readable.
template <typename Key, typename Type>
struct Value<std::map<Key, Type>> {
	static inline
	std::map<Key, Type> read(State* state, int n) {
		std::map<Key, Type> ret;

		// We need to push the table onto the stack because the indices become unpredictable 
		// otherwise.
		lua_pushvalue(state, n);

		lua_pushnil(state);
		while (lua_next(state, -2) != 0) {
			ret.insert(std::make_pair(luwra::read<Key>(state, -2), luwra::read<Type>(state, -1)));

			// Only remove the value, leave the key.
			lua_pop(state, 1);
		}

		// Remove the reference to the table that we pushed first.
		lua_pop(state, 1);

		return ret;
	}

	static inline
	void push(State* state, const std::map<Key, Type>& map) {
		lua_createtable(state, 0, static_cast<int>(map.size()));

		for (const auto& entry: map) {
			luwra::push(state, entry.first);
			luwra::push(state, entry.second);
			lua_rawset(state, -3);
		}
	}
};

namespace internal {
	template <typename... Contents>
	struct _TuplePusher {
		template <size_t... Indices>
		struct Pusher {
			static inline
			size_t push(State* state, const std::tuple<Contents...>& value) {
				return pushReturn(state, std::get<Indices>(value)...);
			}
		};
	};

	template <typename... Contents>
	using TuplePusher =
		typename MakeIndexSequence<sizeof...(Contents)>::template Relay<
			_TuplePusher<Contents...>::template Pusher
		>;
}

/// Enables `std::tuple` as return type
template <typename... Contents>
struct ReturnValue<std::tuple<Contents...>>:
	internal::TuplePusher<Contents...> {};

/// Enables `std::pair` as return type
template <typename First, typename Second>
struct ReturnValue<std::pair<First, Second>> {
	static inline
	size_t push(State* state, const std::pair<First, Second>& value) {
		return pushReturn(state, value.first, value.second);
	}
};

LUWRA_NS_END

#endif

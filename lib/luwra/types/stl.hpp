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
		lua_createtable(state, vec.size(), 0);

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
		lua_createtable(state, lst.size(), 0);

		int i = 0;
		for (const Type& item: lst) {
			luwra::push(state, item);
			lua_rawseti(state, -2, ++i);
		}
	}
};

/// Enables pushing for `std::map` assuming `Key` and `Type` are pushable
template <typename Key, typename Type>
struct Value<std::map<Key, Type>> {
	static inline
	void push(State* state, const std::map<Key, Type>& map) {
		lua_createtable(state, 0, map.size());

		for (const auto& entry: map) {
			luwra::push(state, entry.first);
			luwra::push(state, entry.second);
			lua_rawset(state, -3);
		}
	}
};

namespace internal {
	template <typename Seq, typename...>
	struct _TuplePusher {
		static_assert(
			sizeof(Seq) == -1,
			"Invalid template parameters to _TuplePusher"
		);
	};

	template <size_t... Indices, typename... Contents>
	struct _TuplePusher<IndexSequence<Indices...>, Contents...> {
		static inline
		size_t push(State* state, const std::tuple<Contents...>& value) {
			return pushReturn(state, std::get<Indices>(value)...);
		}
	};

	template <typename... Contents>
	using TuplePusher = _TuplePusher<MakeIndexSequence<sizeof...(Contents)>, Contents...>;
}

/// Enables `std::tuple` as return type
template <typename... Contents>
struct ReturnValue<std::tuple<Contents...>>:
	internal::TuplePusher<Contents...> {};

LUWRA_NS_END

#endif

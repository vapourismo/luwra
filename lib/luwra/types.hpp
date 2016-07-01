/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_TYPES_H_
#define LUWRA_TYPES_H_

#include "common.hpp"
#include "internal/indexsequence.hpp"

#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <list>
#include <map>

LUWRA_NS_BEGIN

using Integer = lua_Integer;
using Number = lua_Number;
using State = lua_State;
using CFunction = lua_CFunction;

template <typename UserType>
struct Value;

/// Alias for @ref Value<Type>
template <typename Type>
struct Value<const Type>: Value<Type> {};

/// Alias for @ref Value<Type>
template <typename Type>
struct Value<volatile Type>: Value<Type> {};

/// Alias for @ref Value<Type>
template <typename Type>
struct Value<const volatile Type>: Value<Type> {};

/// Alias for @ref Value<Type>
template <typename Type>
struct Value<Type&>: Value<Type> {};

/// Alias for @ref Value<Type>
template <typename Type>
struct Value<Type&&>: Value<Type> {};

/// Push a value onto the stack.
///
/// \param state Lua state
/// \param value To be pushed
template <typename Type> inline
void push(State* state, Type&& value) {
	Value<Type>::push(state, std::forward<Type>(value));
}

/// Push two or more values onto the stack.
///
/// \param state  Lua state
/// \param first  First value
/// \param second Second value
/// \param rest   Remaining values
template <typename First, typename Second, typename... Rest> inline
void push(State* state, First&& first, Second&& second, Rest&&... rest) {
	push(state, std::forward<First>(first));
	push(state, std::forward<Second>(second), std::forward<Rest>(rest)...);
}

/// Read a value off the stack.
///
/// \tparam Type  Type of targeted value
/// \param  state Lua state
/// \param  index Position of the value on the stack
template <typename Type> inline
auto read(State* state, int index) -> decltype(Value<Type>::read(state, index)) {
	return Value<Type>::read(state, index);
}

/// Enables reading/pushing `nil`
template <>
struct Value<std::nullptr_t> {
	static inline
	std::nullptr_t read(State* state, int n) {
		luaL_checktype(state, n, LUA_TNIL);
		return nullptr;
	}

	static inline
	void push(State* state, std::nullptr_t) {
		lua_pushnil(state);
	}
};

/// Enables reading Lua threads
template <>
struct Value<State*> {
	static inline
	State* read(State* state, int n) {
		luaL_checktype(state, n, LUA_TTHREAD);
		return lua_tothread(state, n);
	}
};

namespace internal {
	template <typename Type>
	struct NumericTransportValue {
		static_assert(
			sizeof(Type) == -1,
			"Parameter to NumericTransportValue is not a numeric base type"
		);
	};

	// Transport unit `Integer`
	template <>
	struct NumericTransportValue<Integer> {
		static inline
		Integer read(State* state, int index) {
			return luaL_checkinteger(state, index);
		}

		static inline
		void push(State* state, Integer value) {
			lua_pushinteger(state, value);
		}
	};

	// Transport unit `Number`
	template <>
	struct NumericTransportValue<Number> {
		static inline
		Number read(State* state, int index) {
			return luaL_checknumber(state, index);
		}

		static inline
		void push(State* state, Number value) {
			lua_pushnumber(state, value);
		}
	};

	// Base for `Value<Type>` specializations which uses `Transport` as transport unit
	template <typename Type, typename Transport>
	struct NumericValueBase {
		static inline
		Type read(State* state, int index) {
			return static_cast<Type>(NumericTransportValue<Transport>::read(state, index));
		}

		static inline
		void push(State* state, Type value) {
			NumericTransportValue<Transport>::push(state, static_cast<Transport>(value));
		}
	};
}

// Define an integral type which will be transported via `base`.
#define LUWRA_DEF_NUMERIC(base, type) \
	template <> \
	struct Value<type>: internal::NumericValueBase<type, base> {};

// Lua-dependent types
LUWRA_DEF_NUMERIC(Number, float)
LUWRA_DEF_NUMERIC(Number, double)
LUWRA_DEF_NUMERIC(Number, long double)

// Integral types
LUWRA_DEF_NUMERIC(Integer, signed   char)
LUWRA_DEF_NUMERIC(Integer, unsigned char)
LUWRA_DEF_NUMERIC(Integer, signed   short)
LUWRA_DEF_NUMERIC(Integer, unsigned short)
LUWRA_DEF_NUMERIC(Integer, signed   int)
LUWRA_DEF_NUMERIC(Integer, unsigned int)
LUWRA_DEF_NUMERIC(Integer, signed   long int)
LUWRA_DEF_NUMERIC(Integer, unsigned long int)
LUWRA_DEF_NUMERIC(Integer, signed   long long int)
LUWRA_DEF_NUMERIC(Integer, unsigned long long int)

// Do not export this macros
#undef LUWRA_DEF_NUMERIC

/// Enables reading/pushing strings as C strings
template <>
struct Value<const char*> {
	static inline
	const char* read(State* state, int n) {
		return luaL_checkstring(state, n);
	}

	static inline
	void push(State* state, const char* value) {
		lua_pushstring(state, value);
	}
};

/// Enables reading/pushing string as `std::string`.
template <>
struct Value<std::string> {
	static inline
	std::string read(State* state, int n) {
		size_t length;
		const char* value = luaL_checklstring(state, n, &length);
		return {value, length};
	}

	static inline
	void push(State* state, const std::string& value) {
		lua_pushstring(state, value.c_str());
	}
};

/// Enables reading/pushing booleans
template <>
struct Value<bool> {
	static inline
	bool read(State* state, int n) {
		luaL_checktype(state, n, LUA_TBOOLEAN);
		return lua_toboolean(state, n) == 1;
	}

	static inline
	void push(State* state, bool value) {
		lua_pushboolean(state, static_cast<int>(value));
	}
};

/// Alias for @ref Value<const char*>
template <>
struct Value<char*>: Value<const char*> {};

/// Alias for @ref Value<const char*>
template <size_t N>
struct Value<char[N]>: Value<const char*> {};

/// Alias for @ref Value<const char*>
template <size_t N>
struct Value<const char[N]>: Value<const char*> {};

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

/// A version of @ref Value for pushing return values onto the stack. @ref ReturnValue inherits
/// `push` implementations from @ref Value.
template <typename Type>
struct ReturnValue {
	template <typename... Args> static inline
	size_t push(State* state, Args&&... args) {
		Value<Type>::push(state, std::forward<Args>(args)...);
		return 1;
	}
};

/// Alias for `ReturnValue<Type>`
template <typename Type>
struct ReturnValue<const Type>: ReturnValue<Type> {};

/// Alias for `ReturnValue<Type>`
template <typename Type>
struct ReturnValue<volatile Type>: ReturnValue<Type> {};

/// Alias for `ReturnValue<Type>`
template <typename Type>
struct ReturnValue<const volatile Type>: ReturnValue<Type> {};

/// Alias for `ReturnValue<Type>`
template <typename Type>
struct ReturnValue<Type&>: ReturnValue<Type> {};

/// Alias for `ReturnValue<Type>`
template <typename Type>
struct ReturnValue<Type&&>: ReturnValue<Type> {};

/// Push a return value onto the stack.
///
/// \param state Lua state
/// \param value Return value
/// \returns Number of Lua values that have been pushed onto the stack
template <typename Type> inline
size_t pushReturn(State* state, Type&& value) {
	return ReturnValue<Type>::push(state, std::forward<Type>(value));
}

/// Push multiple return values onto the stack.
///
/// \param state   Lua state
/// \param first   First return value
/// \param second  Second return value
/// \param rest    More return values
/// \returns Number of Lua values that have been pushed onto the stack
template <typename First, typename Second, typename... Rest> inline
size_t pushReturn(State* state, First&& first, Second&& second, Rest&&... rest) {
	return
		pushReturn(state, std::forward<First>(first)) +
		pushReturn(state, std::forward<Second>(second), std::forward<Rest>(rest)...);
}

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

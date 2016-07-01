/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_VALUES_H_
#define LUWRA_VALUES_H_

#include "common.hpp"
#include "internal/indexsequence.hpp"

#include <utility>
#include <string>

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

LUWRA_NS_END

#endif

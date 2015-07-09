/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_TYPES_H_
#define LUWRA_TYPES_H_

#include "common.hpp"

#include <utility>
#include <tuple>
#include <string>
#include <type_traits>
#include <limits>

LUWRA_NS_BEGIN

/* Lua types */
using Integer = lua_Integer;
using Number = lua_Number;
using State = lua_State;
using CFunction = lua_CFunction;

/**
 * A value on the stack
 */
template <typename T>
struct Value {
	static_assert(
		sizeof(T) == -1,
		"Parameter to Value is not supported"
	);
};


// Nil
template <>
struct Value<std::nullptr_t> {
	static inline
	std::nullptr_t read(State* state, int n) {
		luaL_checktype(state, n, LUA_TNIL);
		return nullptr;
	}

	static inline
	int push(State* state, std::nullptr_t) {
		lua_pushnil(state);
		return 1;
	}
};

/**
 * Convenient wrapped for `Value<T>::push`.
 */
template <typename T> static inline
int push(State* state, T value) {
	return Value<T>::push(state, value);
}

/**
 * Convenient wrapper for `Value<T>::read`.
 */
template <typename T> static inline
T read(State* state, int index) {
	return Value<T>::read(state, index);
}

/**
 * Define a template specialization of `Value` for `type` with a `retrf(State*, int)` which
 * extracts it from the stack and a `pushf(State*, type)` which pushes the value on the stack again.
 * This assumes that only one value will be pushed onto the stack.
 */
#define LUWRA_DEF_VALUE(type, retrf, pushf)                          \
	template <>                                                      \
	struct Value<type> {                                             \
		static inline                                                \
		type read(State* state, int n) {                             \
			return retrf(state, n);                                  \
		}                                                            \
                                                                     \
		static inline                                                \
		int push(State* state, type value) {                         \
			pushf(state, value);                                     \
			return 1;                                                \
		}                                                            \
	}

#ifndef luaL_checkboolean
	/**
	 * Check if the value at index `n` is a boolean and retrieve its value.
	 */
	#define luaL_checkboolean(state, n) \
		(luaL_checktype(state, n, LUA_TBOOLEAN), lua_toboolean(state, n))
#endif

#ifndef luaL_checkcfunction
	/**
	 * Check if the value at index `n` is a C function and retrieve it.
	 */
	#define luaL_checkcfunction(state, n) \
		(luaL_checktype(state, n, LUA_TCFUNCTION), lua_toboolean(state, n))
#endif

#ifndef luaL_pushstdstring
	/**
	 * push a `std::string` as string onto the stack.
	 */
	#define luaL_pushstdstring(state, stdstring) \
		(lua_pushstring(state, (stdstring).c_str()))
#endif

namespace internal {
	template <typename T>
	struct NumericTransportValue {
		static_assert(
			sizeof(T) == -1,
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
		int push(State* state, Integer value) {
			lua_pushinteger(state, value);
			return 1;
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
		int push(State* state, Number value) {
			lua_pushnumber(state, value);
			return 1;
		}
	};

	// Base for `Value<I>` specializations which uses `B` as transport unit, where `I` is smaller
	// than `B`.
	template <typename I, typename B>
	struct NumericContainedValueBase {
		static constexpr
		bool qualifies =
			// TODO: Remove warning about comparsion between signed and unsigned integers
			std::numeric_limits<I>::max() <= std::numeric_limits<B>::max()
			&& std::numeric_limits<I>::lowest() >= std::numeric_limits<B>::lowest();

		static inline
		I read(State* state, int index) {
			return
				static_cast<I>(NumericTransportValue<B>::read(state, index));
		}

		static inline
		int push(State* state, I value) {
			NumericTransportValue<B>::push(state, static_cast<B>(value));
			return 1;
		}
	};

	// Base for `Value<I>` specializations which uses `B` as transport unit, where `I` is bigger
	// than `B`.
	template <typename I, typename B>
	struct NumericTruncatingValueBase {
		static inline
		I read(State* state, int index) {
			return static_cast<I>(NumericTransportValue<B>::read(state, index));
		}

		static inline
		int push(State*, I) {
			static_assert(
				sizeof(I) == -1,
				"You must not use 'Value<I>::push' specializations which inherit from NumericTruncatingValueBase"
			);

			return -1;
		}
	};

	// Base for `Value<I>` specializations which uses `B` as transport unit
	template <typename I, typename B>
	using NumericValueBase =
		typename std::conditional<
			std::is_same<I, B>::value,
			NumericTransportValue<B>,
			typename std::conditional<
				NumericContainedValueBase<I, B>::qualifies,
				NumericContainedValueBase<I, B>,
				NumericTruncatingValueBase<I, B>
			>::type
		>::type;
}

/**
 * Define an integral type which will be transported via `base`.
 */
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

// C/C++ types
LUWRA_DEF_VALUE(bool,        luaL_checkboolean, lua_pushboolean);
LUWRA_DEF_VALUE(const char*, luaL_checkstring,  lua_pushstring);
LUWRA_DEF_VALUE(std::string, luaL_checkstring,  luaL_pushstdstring);

// Do not export these macros
#undef LUWRA_DEF_VALUE
#undef LUWRA_DEF_NUMERIC

// Alias for string literals
template <size_t n>
struct Value<char[n]>: Value<const char*> {};

// Alias for const string literals
template <size_t n>
struct Value<const char[n]>: Value<const char*> {};

/**
 * C Functions may be pushed aswell.
 */
template <>
struct Value<CFunction> {
	static inline
	int push(State* state, CFunction fun) {
		lua_pushcfunction(state, fun);
		return 1;
	}
};

/**
 * An arbitrary value on an execution stack.
 * Note: this value is only available as long as it exists on its originating stack.
 */
struct Arbitrary {
	State* state;
	int index;
};

template <>
struct Value<Arbitrary> {
	static inline
	Arbitrary read(State* state, int index) {
		if (index < 0)
			index = lua_gettop(state) + (index + 1);

		return Arbitrary {state, index};
	}

	static inline
	int push(State* state, const Arbitrary& value) {
		lua_pushvalue(value.state, value.index);

		if (value.state != state)
			lua_xmove(value.state, state, 1);

		return 1;
	}
};

namespace internal {
	template <typename>
	struct StackPusher;

	template <size_t I>
	struct StackPusher<std::index_sequence<I>> {
		template <typename... T> static inline
		int push(State* state, const std::tuple<T...>& package) {
			using R = typename std::tuple_element<I, std::tuple<T...>>::type;
			return std::max(0, Value<R>::push(state, std::get<I>(package)));
		}
	};

	template <size_t I, size_t... Is>
	struct StackPusher<std::index_sequence<I, Is...>> {
		template <typename... T> static inline
		int push(State* state, const std::tuple<T...>& package) {
			return
				StackPusher<std::index_sequence<I>>::push(state, package)
				+ StackPusher<std::index_sequence<Is...>>::push(state, package);
		}
	};
}

/**
 * Allows you to use multiple return values.
 */
template <typename... A>
struct Value<std::tuple<A...>> {
	static inline
	int push(State* state, const std::tuple<A...>& value) {
		return internal::StackPusher<std::make_index_sequence<sizeof...(A)>>::push(state, value);
	}
};

/**
 * Fix specialization for const types.
 */
template <typename T>
struct Value<const T>: Value<T> {};

/**
 * Fix specialization for volatile types.
 */
template <typename T>
struct Value<volatile T>: Value<T> {};

LUWRA_NS_END

#endif

/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_TYPES_H_
#define LUWRA_TYPES_H_

#include "common.h"

#include <utility>
#include <tuple>
#include <string>

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
	static_assert(sizeof(T) == -1, "You must not use an unspecialized version of Value");

	/**
	 * Retrieve the value at position `n`.
	 */
	static
	T read(State*, int);

	/**
	 * Push the value onto the stack.
	 */
	static
	int push(State*, T);
};

/**
 * Define a template specialization of `Value` for `type` with a `retrf(State*, int)` which
 * extracts it from the stack and a `pushf(State*, type)` which pushes the value on the stack again.
 * This assumes that only one value will be pushed onto the stack.
 */
#define LUWRA_DEF_VALUE(type, retrf, pushf)                       \
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

#ifndef luaL_pushstdstring
	/**
	 * Push a `std::string` as string onto the stack.
	 */
	#define luaL_pushstdstring(state, stdstring) \
		(lua_pushstring(state, stdstring.c_str()))
#endif

/* Lua-dependent types */
LUWRA_DEF_VALUE(Integer,     luaL_checkinteger, lua_pushinteger);
LUWRA_DEF_VALUE(Number,      luaL_checknumber,  lua_pushnumber);

/* C/C++ types */
LUWRA_DEF_VALUE(bool,        luaL_checkboolean, lua_pushboolean);
LUWRA_DEF_VALUE(const char*, luaL_checkstring,  lua_pushstring);
LUWRA_DEF_VALUE(std::string, luaL_checkstring,  luaL_pushstdstring);

#undef LUWRA_DEF_VALUE

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
		template <typename T> static inline
		void push(State* state, const T& package) {
			Value<typename std::tuple_element<I, T>::type>::push(state, std::get<I>(package));
		}
	};

	template <size_t I, size_t... Is>
	struct StackPusher<std::index_sequence<I, Is...>> {
		template <typename T> static inline
		void push(State* state, const T& package) {
			Value<typename std::tuple_element<I, T>::type>::push(state, std::get<I>(package));
			StackPusher<std::index_sequence<Is...>>::push(state, package);
		}
	};
}

/**
 * Allows you to use multiple return values.
 */
template <typename... A>
struct Value<std::tuple<A...>> {
	static inline
	std::tuple<A...> read(State*, int) {
		static_assert(sizeof(std::tuple<A...>) == -1, "std::tuples cannot be read from the stack");
	}

	static inline
	int push(State* state, const std::tuple<A...>& value) {
		internal::StackPusher<std::make_index_sequence<sizeof...(A)>>::push(state, value);
		return sizeof...(A);
	}
};

LUWRA_NS_END

#endif

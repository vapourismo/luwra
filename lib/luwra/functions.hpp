/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_FUNCTIONS_H_
#define LUWRA_FUNCTIONS_H_

#include "common.hpp"
#include "types.hpp"
#include "stack.hpp"

LUWRA_NS_BEGIN

namespace internal {
	template <typename T>
	struct FunctionWrapper {
		static_assert(
			sizeof(T) == -1,
			"Parameter to FunctionWrapper is not a valid signature"
		);
	};

	template <typename R, typename... A>
	struct FunctionWrapper<R(A...)> {
		template <R (* fun)(A...)> static inline
		int invoke(State* state) {
			return map<R(A...)>(state, fun);
		}
	};

	// We need an alias, because function pointers are weird
	template <typename R, typename... A>
	struct FunctionWrapper<R(*)(A...)>: FunctionWrapper<R(A...)> {};
}

/**
 * A callable native Lua function.
 * \note This value is only available as long as it exists on the stack.
 */
template <typename R>
struct NativeFunction: Arbitrary {
	NativeFunction(State* state, int index):
		Arbitrary(state, index)
	{}

	template <typename... A> inline
	R operator ()(A&&... args) {
		push<Arbitrary>(state, *this);
		size_t numArgs = push(state, std::forward<A>(args)...);

		lua_call(state, numArgs, 1);
		R returnValue = read<R>(state, -1);

		lua_pop(state, 1);
		return returnValue;
	}
};

/**
 * A callable native Lua function.
 * \note This value is only available as long as it exists on the stack.
 */
template <>
struct NativeFunction<void>: Arbitrary {
	NativeFunction(State* state, int index):
		Arbitrary(state, index)
	{}

	template <typename... A> inline
	void operator ()(A&&... args) {
		push<Arbitrary>(state, *this);
		size_t numArgs = push(state, std::forward<A>(args)...);

		lua_call(state, numArgs, 0);
	}
};

template <typename R>
struct Value<NativeFunction<R>> {
	static inline
	NativeFunction<R> read(State* state, int index) {
		luaL_checktype(state, index, LUA_TFUNCTION);
		return NativeFunction<R>(state, index);
	}
};

LUWRA_NS_END

/**
 * Generate a `lua_CFunction` wrapper for a function.
 * \param fun Fully qualified function name (Do not supply a pointer)
 * \returns Wrapped function as `lua_CFunction`
 */
#define LUWRA_WRAP_FUNCTION(fun) \
	(&luwra::internal::FunctionWrapper<decltype(&fun)>::template invoke<&fun>)

#endif

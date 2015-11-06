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

template <typename S>
struct NativeFunction {
	static_assert(
		sizeof(S) == -1,
		"Parameter to NativeFunction is not a valid signature"
	);
};

/**
 * A callable native Lua function.
 * \note This value is only as long as it exists on the stack.
 */
template <typename R, typename... A>
struct NativeFunction<R(A...)> {
	State* state;
	int index;

	inline
	R operator ()(A&&... args) {
		lua_pushvalue(state, index);
		size_t numArgs = push(state, std::forward<A>(args)...);

		lua_call(state, numArgs, 1);
		R returnValue = read<R>(state, -1);

		lua_pop(state, 1);
		return returnValue;
	}
};

/**
 * A callable native Lua function.
 * \note This value is only as long as it exists on the stack.
 */
template <typename... A>
struct NativeFunction<void(A...)> {
	State* state;
	int index;

	inline
	void operator ()(A&&... args) {
		lua_pushvalue(state, index);
		size_t numArgs = push(state, std::forward<A>(args)...);

		lua_call(state, numArgs, 0);
	}
};

template <typename R, typename... A>
struct Value<NativeFunction<R(A...)>> {
	static inline
	NativeFunction<R(A...)> read(State* state, int index) {
		luaL_checktype(state, index, LUA_TFUNCTION);
		return {state, index};
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

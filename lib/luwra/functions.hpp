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
			return static_cast<int>(
				map<R(A...)>(state, fun)
			);
		}
	};

	// We need an alias, because function pointers are weird
	template <typename R, typename... A>
	struct FunctionWrapper<R (*)(A...)>: FunctionWrapper<R (A...)> {};
}

/**
 * A callable native Lua function.
 * \note This value is only available as long as it exists on the stack.
 */
template <typename R>
struct NativeFunction: Reference {
	NativeFunction(State* state, int index):
		Reference(state, index)
	{}

	template <typename... A> inline
	R operator ()(A&&... args) {
		impl->push();
		size_t numArgs = push(impl->state, std::forward<A>(args)...);

		lua_call(impl->state, static_cast<int>(numArgs), 1);
		R returnValue = Value<R>::read(impl->state, -1);

		lua_pop(impl->state, 1);
		return returnValue;
	}
};

/**
 * A callable native Lua function.
 * \note This value is only available as long as it exists on the stack.
 */
template <>
struct NativeFunction<void>: Reference {
	NativeFunction(State* state, int index):
		Reference(state, index)
	{}

	template <typename... A> inline
	void operator ()(A&&... args) {
		impl->push();
		size_t numArgs = push(impl->state, std::forward<A>(args)...);

		lua_call(impl->state, static_cast<int>(numArgs), 0);
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

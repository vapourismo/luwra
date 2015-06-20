/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_WRAPPERS_H_
#define LUWRA_WRAPPERS_H_

#include "common.hpp"
#include "types.hpp"
#include "stack.hpp"

LUWRA_NS_BEGIN

namespace internal {
	template <typename T>
	struct FunctionWrapper {
		static_assert(
			sizeof(T) == -1,
			"The FunctionWrapper template expects a function signature as parameter"
		);
	};

	template <>
	struct FunctionWrapper<void()> {
		template <void(*funptr)()> static
		int invoke(State*) {
			funptr();
			return 0;
		}
	};

	template <typename R>
	struct FunctionWrapper<R()> {
		template <R(*funptr)()> static
		int invoke(State* state) {
			return Value<R>::push(state, funptr());
		}
	};

	template <typename... A>
	struct FunctionWrapper<void(A...)> {
		template <void (*funptr)(A...)> static
		int invoke(State* state) {
			apply(state, funptr);
			return 0;
		}
	};

	template <typename R, typename... A>
	struct FunctionWrapper<R(A...)> {
		template <R (*funptr)(A...)> static
		int invoke(State* state) {
			return Value<R>::push(
				state,
				apply(state, funptr)
			);
		}
	};
}

/**
 * Assuming its parameters can be retrieved from the Lua stack, ordinary functions can be wrapped
 * using the `WrapFunction` instance in order to produce a C function which can be used by the
 * Lua VM.
 *
 * Assuming your function has the following signature:
 *
 *   R my_fun(A0, A1 ... An);
 *
 * Generate a Lua-compatible like so:
 *
 *   CFunction wrapped_fun = WrapFunction<R(A0, A1 ... An), my_fun>;
 */
template <
	typename S,
	S* FunctionPointer
>
constexpr CFunction WrapFunction =
	&internal::FunctionWrapper<S>::template invoke<FunctionPointer>;

LUWRA_NS_END

#endif

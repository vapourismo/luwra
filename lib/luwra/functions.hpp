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
			"The FunctionWrapper template expects a function signature as parameter"
		);
	};

	template <>
	struct FunctionWrapper<void()> {
		template <void(*FunctionPointer)()> static
		int Invoke(State*) {
			FunctionPointer();
			return 0;
		}
	};

	template <typename R>
	struct FunctionWrapper<R()> {
		template <R(*FunctionPointer)()> static
		int Invoke(State* state) {
			return Value<R>::Push(state, FunctionPointer());
		}
	};

	template <typename... A>
	struct FunctionWrapper<void(A...)> {
		template <void (*FunctionPointer)(A...)> static
		int Invoke(State* state) {
			apply(state, FunctionPointer);
			return 0;
		}
	};

	template <typename R, typename... A>
	struct FunctionWrapper<R(A...)> {
		template <R (*FunctionPointer)(A...)> static
		int Invoke(State* state) {
			return Value<R>::Push(
				state,
				apply(state, FunctionPointer)
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
	&internal::FunctionWrapper<S>::template Invoke<FunctionPointer>;

LUWRA_NS_END

#endif

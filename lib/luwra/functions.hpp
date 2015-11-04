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
			"Parameter to FunctionWrapper is not a function signature"
		);
	};

	template <typename... A>
	struct FunctionWrapper<void(A...)> {
		template <void (*function_pointer)(A...)> static inline
		int invoke(State* state) {
			apply(state, function_pointer);
			return 0;
		}
	};

	template <typename R, typename... A>
	struct FunctionWrapper<R(A...)> {
		template <R (*function_pointer)(A...)> static inline
		int invoke(State* state) {
			return push(
				state,
				apply(state, function_pointer)
			);
		}
	};

	template <typename T>
	struct FunctionWrapperHelper {
		static_assert(
			sizeof(T) == -1,
			"Parameter to FunctionWrapperHelper is not a function pointer"
		);
	};

	template <typename R, typename... A>
	struct FunctionWrapperHelper<R(*)(A...)> {
		using Signature = R(A...);
	};
}

template <
	typename S,
	S* function_pointer
>
constexpr CFunction wrap_function =
	&internal::FunctionWrapper<S>::template invoke<function_pointer>;

#define LUWRA_WRAP_FUNCTION(fun) \
	(luwra::wrap_function< \
	     typename luwra::internal::FunctionWrapperHelper<decltype(&fun)>::Signature, \
	     &fun \
	 >)

LUWRA_NS_END

#endif

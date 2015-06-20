/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_STACK_H_
#define LUWRA_STACK_H_

#include "common.hpp"
#include "types.hpp"

#include <utility>
#include <functional>
#include <type_traits>

LUWRA_NS_BEGIN

namespace internal {
	template <typename>
	struct Layout;

	template <typename R, typename T>
	struct Layout<R(T)> {
		template <typename F, typename... A> static inline
		R direct(State* state, int n, F hook, A&&... args) {
			return hook(
				std::forward<A>(args)...,
				Value<T>::read(state, n)
			);
		}
	};

	template <typename R, typename T1, typename... TR>
	struct Layout<R(T1, TR...)> {
		template <typename F, typename... A> static inline
		R direct(State* state, int n, F hook, A&&... args) {
			return Layout<TR...>::template direct<F, R>(
				state,
				n + 1,
				hook,
				std::forward<A>(args)...,
				Value<T1>::read(state, n)
			);
		}
	};
}

/**
 * Assuming a stack layout as follows (where A = A0, A1 ... An):
 *
 *   Position | Parameter
 * 	----------+-----------
 *   pos + n  | An xn
 *   ...      | ...
 *   pos + 1  | A1 x1
 *   pos + 0  | A0 x0
 *   ...      | ...
 *
 * Given a function `R f(A0, A1, ... An)` you are able to map over
 * the values on the stack on the stack like so:
 *
 *   R my_result = apply(lua_state, pos, f);
 *
 * which is equivalent to
 *
 *   R my_result = f(x0, x1, ... xn);
 *
 * where x0, x1, ... xn are the values on the stack.
 */
template <typename R, typename... A> static inline
R apply(State* state, int pos, R (*funptr)(A...)) {
	return internal::Layout<R(A...)>::direct(state, pos, funptr);
}

/**
 * Same as `apply(state, 1, funptr)`.
 */
template <typename R, typename... A> static inline
R apply(State* state, R (*funptr)(A...)) {
	return apply(state, 1, funptr);
}

/**
 * Specialization of `apply` which works for `std::function`.
 */
template <typename R, typename... A> static inline
R apply(State* state, int pos, std::function<R(A...)> fun) {
	return internal::Layout<R(A...)>::direct(state, pos, fun);
}

/**
 * Same as `apply(state, 1, fun)`.
 */
template <typename R, typename... A> static inline
R apply(State* state, std::function<R(A...)> fun) {
	return apply(state, 1, fun);
}

LUWRA_NS_END

#endif

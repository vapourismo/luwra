/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_STACK_H_
#define LUWRA_STACK_H_

#include "common.hpp"
#include "types.hpp"

#include <cassert>
#include <utility>
#include <functional>
#include <string>

LUWRA_NS_BEGIN

namespace internal {
	template <typename T>
	struct Layout {
		static_assert(
			sizeof(T) == -1,
			"Parameter to Layout is not a function signature"
		);
	};

	template <typename R>
	struct Layout<R()> {
		using ReturnType = R;

		template <typename F, typename... A> static inline
		R direct(State*, int, F hook, A&&... args) {
			return hook(
				std::forward<A>(args)...
			);
		}
	};

	template <typename R, typename T>
	struct Layout<R(T)> {
		using ReturnType = R;

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
		using ReturnType = R;

		template <typename F, typename... A> static inline
		R direct(State* state, int n, F hook, A&&... args) {
			return Layout<R(TR...)>::direct(
				state,
				n + 1,
				hook,
				std::forward<A>(args)...,
				Value<T1>::read(state, n)
			);
		}
	};

	template <typename K, typename V, typename... R>
	struct EntryPusher {
		static inline
		void push(State* state, int index, K&& key, V&& value, R&&... rest) {
			EntryPusher<K, V>::push(state, index, std::forward<K>(key), std::forward<V>(value));
			EntryPusher<R...>::push(state, index, std::forward<R>(rest)...);
		}
	};

	template <typename K, typename V>
	struct EntryPusher<K, V> {
		static inline
		void push(State* state, int index, K&& key, V&& value) {
			assert(1 == luwra::push(state, key));
			assert(1 == luwra::push(state, value));
			lua_rawset(state, index < 0 ? index - 2 : index);
		}
	};
}

/**
 * \todo Document me
 */
template <typename S, typename F> static inline
typename internal::Layout<S>::ReturnType direct(State* state, int pos, F hook) {
	return internal::Layout<S>::direct(state, pos, hook);
}

/**
 * Same as `direct(state, 1, hook)`.
 */
template <typename S, typename F> static inline
typename internal::Layout<S>::ReturnType direct(State* state, F hook) {
	return internal::Layout<S>::direct(state, 1, hook);
}

/**
 * Assuming a stack layout as follows (where A = A0, A1 ... An):
 *
 *   Position | Type | Identifier
 * 	----------+------+------------
 *   pos + 0  | A0   | x0
 *   pos + 1  | A1   | x1
 *   ...      | ...  | ...
 *   pos + n  | An   | xn
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
R apply(State* state, int pos, R (* function_pointer)(A...)) {
	return direct<R(A...)>(state, pos, function_pointer);
}

/**
 * Same as `apply(state, 1, function_pointer)`.
 */
template <typename R, typename... A> static inline
R apply(State* state, R (* function_pointer)(A...)) {
	return apply(state, 1, function_pointer);
}

/**
 * Specialization of `apply` which works with `std::function`.
 */
template <typename R, typename... A> static inline
R apply(State* state, int pos, const std::function<R(A...)>& function_object) {
	return internal::Layout<R(A...)>::direct(state, pos, function_object);
}

/**
 * Same as `apply(state, 1, function_object)`.
 */
template <typename R, typename... A> static inline
R apply(State* state, const std::function<R(A...)>& function_object) {
	return apply(state, 1, function_object);
}

LUWRA_NS_END

#endif

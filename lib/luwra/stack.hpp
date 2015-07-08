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

LUWRA_NS_BEGIN

namespace internal {
	template <typename T>
	struct Layout {
		static_assert(
			sizeof(T) == -1,
			"Parameter to Layout is not a function signature"
		);
	};

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
R apply(State* state, int pos, R (*function_pointer)(A...)) {
	return internal::Layout<R(A...)>::direct(state, pos, function_pointer);
}

/**
 * Same as `apply(state, 1, function_pointer)`.
 */
template <typename R, typename... A> static inline
R apply(State* state, R (*function_pointer)(A...)) {
	return apply(state, 1, function_pointer);
}

/**
 * Specialization of `apply` which works for `std::function`.
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

/**
 * Check if two values are equal.
 */
static inline
bool equal(State* state, int index1, int index2) {
#if LUA_VERSION_NUM <= 501
	return lua_equal(state, index1, index2);
#else
	return lua_compare(state, index1, index2, LUA_OPEQ);
#endif
}

/**
 * Register a value as a global.
 */
template <typename T> static inline
void register_global(State* state, const char* name, T value) {
	assert(1 == push(state, value));
	lua_setglobal(state, name);
}

/**
 * Set multiple fields at once. Allows you to provide multiple key-value pairs.
 */
template <typename... R> static inline
void set_fields(State* state, int index, R&&... args) {
	static_assert(sizeof...(R) % 2 == 0, "Field parameters must appear in pairs");
	internal::EntryPusher<R...>::push(state, index, std::forward<R>(args)...);
}

/**
 * Create a new table and set its fields.
 */
template <typename... R> static inline
void new_table(State* state, R&&... args) {
	lua_newtable(state);
	set_fields(state, lua_gettop(state), std::forward<R>(args)...);
}

LUWRA_NS_END

#endif

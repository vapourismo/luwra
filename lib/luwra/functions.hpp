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
#include "usertypes.hpp"

#include <functional>

LUWRA_NS_BEGIN

/**
 * A callable native Lua function.
 */
template <typename R>
struct NativeFunction {
	Reference ref;

	inline
	NativeFunction(const Reference& ref):
		ref(ref)
	{}

	inline
	NativeFunction(State* state, int index):
		ref(state, index)
	{}

	template <typename T> inline
	NativeFunction(const NativeFunction<T>& other):
		ref(other.ref)
	{}

	inline
	R operator ()() const {
		ref.impl->push();

		lua_call(ref.impl->state, 0, 1);
		R returnValue = read<R>(ref.impl->state, -1);

		lua_pop(ref.impl->state, 1);
		return returnValue;
	}

	template <typename... A> inline
	R operator ()(A&&... args) const {
		ref.impl->push();
		size_t numArgs = push(ref.impl->state, std::forward<A>(args)...);

		lua_call(ref.impl->state, static_cast<int>(numArgs), 1);
		R returnValue = read<R>(ref.impl->state, -1);

		lua_pop(ref.impl->state, 1);
		return returnValue;
	}
};

/**
 * A callable native Lua function.
 */
template <>
struct NativeFunction<void> {
	Reference ref;

	inline
	NativeFunction(const Reference& ref):
		ref(ref)
	{}

	inline
	NativeFunction(State* state, int index):
		ref(state, index)
	{
		int type = lua_type(state, index);
		if (type != LUA_TTABLE && type != LUA_TUSERDATA && type != LUA_TFUNCTION)
			luaL_argerror(state, index, "Expected table, userdata or function");
	}

	inline
	void operator ()() const {
		ref.impl->push();
		lua_call(ref.impl->state, 0, 0);
	}

	template <typename... A> inline
	void operator ()(A&&... args) const {
		ref.impl->push();
		size_t numArgs = push(ref.impl->state, std::forward<A>(args)...);

		lua_call(ref.impl->state, static_cast<int>(numArgs), 0);
	}
};

template <typename R>
struct Value<NativeFunction<R>> {
	static inline
	NativeFunction<R> read(State* state, int index) {
		return {state, index};
	}

	static inline
	size_t push(State* state, const NativeFunction<R>& func) {
		return Value<Reference>::push(state, func);
	}
};

template <typename R, typename... A>
struct Value<std::function<R(A...)>> {
	static inline
	std::function<R(A...)> read(State* state, int index) {
		return {Value<NativeFunction<R>>::read(state, index)};
	}
};

LUWRA_NS_END

#endif

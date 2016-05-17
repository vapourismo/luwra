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

/**
 * A callable native Lua function.
 */
template <typename R>
struct NativeFunction: Reference {
	NativeFunction(State* state, int index):
		Reference(state, index)
	{}

	inline
	R operator ()() const {
		impl->push();

		lua_call(impl->state, 0, 1);
		R returnValue = Value<R>::read(impl->state, -1);

		lua_pop(impl->state, 1);
		return returnValue;
	}

	template <typename... A> inline
	R operator ()(A&&... args) const {
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
 */
template <>
struct NativeFunction<void>: Reference {
	NativeFunction(State* state, int index):
		Reference(state, index)
	{}

	inline
	void operator ()() const {
		impl->push();
		lua_call(impl->state, 0, 0);
	}

	template <typename... A> inline
	void operator ()(A&&... args) const {
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
		return {state, index};
	}
};

LUWRA_NS_END

#endif

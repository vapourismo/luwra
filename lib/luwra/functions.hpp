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
template <typename Ret>
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

	template <typename OtherRet> inline
	NativeFunction(const NativeFunction<OtherRet>& other):
		ref(other.ref)
	{}

	inline
	Ret operator ()() const {
		ref.impl->push();

		lua_call(ref.impl->state, 0, 1);
		Ret returnValue = read<Ret>(ref.impl->state, -1);

		lua_pop(ref.impl->state, 1);
		return returnValue;
	}

	template <typename... Args> inline
	Ret operator ()(Args&&... args) const {
		ref.impl->push();
		push(ref.impl->state, std::forward<Args>(args)...);

		lua_call(ref.impl->state, sizeof...(Args), 1);
		Ret returnValue = read<Ret>(ref.impl->state, -1);

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

	template <typename... Args> inline
	void operator ()(Args&&... args) const {
		ref.impl->push();
		push(ref.impl->state, std::forward<Args>(args)...);

		lua_call(ref.impl->state, sizeof...(Args), 0);
	}
};

template <typename Ret>
struct Value<NativeFunction<Ret>> {
	static inline
	NativeFunction<Ret> read(State* state, int index) {
		return {state, index};
	}

	static inline
	void push(State* state, const NativeFunction<Ret>& func) {
		Value<Reference>::push(state, func);
	}
};

template <typename Ret, typename... Args>
struct Value<std::function<Ret (Args...)>> {
	static inline
	std::function<Ret (Args...)> read(State* state, int index) {
		return {Value<NativeFunction<Ret>>::read(state, index)};
	}
};

template <>
struct Value<CFunction> {
	static inline
	void push(State* state, CFunction fun) {
		lua_pushcfunction(state, fun);
	}
};

LUWRA_NS_END

#endif

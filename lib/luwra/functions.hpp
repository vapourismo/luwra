/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_FUNCTIONS_H_
#define LUWRA_FUNCTIONS_H_

#include "common.hpp"
#include "values.hpp"
#include "types/reference.hpp"
#include "stack.hpp"

#include <utility>
#include <functional>

LUWRA_NS_BEGIN

/// A callable Lua function.
///
/// \tparam Ret Expected return type
template <typename Ret>
struct NativeFunction {
	Reference ref;

	/// Create from reference.
	inline
	NativeFunction(const Reference& ref):
		ref(ref)
	{}

	/// Create from function on the stack.
	inline
	NativeFunction(State* state, int index):
		ref(state, index)
	{}

	/// Convert from an existing @ref NativeFunction.
	template <typename OtherRet> inline
	NativeFunction(const NativeFunction<OtherRet>& other):
		ref(other.ref)
	{}

	/// Invoke the function with no arguments.
	inline
	Ret operator ()() const {
		ref.impl->push();

		lua_call(ref.impl->state, 0, 1);
		Ret returnValue = read<Ret>(ref.impl->state, -1);

		lua_pop(ref.impl->state, 1);
		return returnValue;
	}

	/// Invoke the function with arguments.
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

/// A callable Lua function without a return value.
template <>
struct NativeFunction<void> {
	Reference ref;

	/// Create from reference.
	inline
	NativeFunction(const Reference& ref):
		ref(ref)
	{}

	/// Create from function on the stack.
	inline
	NativeFunction(State* state, int index):
		ref(state, index)
	{
		int type = lua_type(state, index);
		if (type != LUA_TTABLE && type != LUA_TUSERDATA && type != LUA_TFUNCTION)
			luaL_argerror(state, index, "Expected table, userdata or function");
	}

	/// Invoke the function with no arguments.
	inline
	void operator ()() const {
		ref.impl->push();
		lua_call(ref.impl->state, 0, 0);
	}

	/// Invoke the function with arguments.
	template <typename... Args> inline
	void operator ()(Args&&... args) const {
		ref.impl->push();
		push(ref.impl->state, std::forward<Args>(args)...);

		lua_call(ref.impl->state, sizeof...(Args), 0);
	}
};

/// Enables reading/pushing Lua functions
template <typename Ret>
struct Value<NativeFunction<Ret>> {
	static inline
	NativeFunction<Ret> read(State* state, int index) {
		return {state, index};
	}

	static inline
	void push(State* state, const NativeFunction<Ret>& func) {
		luwra::push(state, func.ref);
	}
};

/// Enables reading Lua functions as `std::function`
template <typename Ret, typename... Args>
struct Value<std::function<Ret (Args...)>> {
	static inline
	std::function<Ret (Args...)> read(State* state, int index) {
		return {Value<NativeFunction<Ret>>::read(state, index)};
	}
};

/// Enables pushing for C functions
template <>
struct Value<CFunction> {
	static inline
	void push(State* state, CFunction fun) {
		lua_pushcfunction(state, fun);
	}
};

LUWRA_NS_END

#endif

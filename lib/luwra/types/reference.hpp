/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_TYPES_REFERENCE_H_
#define LUWRA_TYPES_REFERENCE_H_

#include "../common.hpp"
#include "../values.hpp"

#include <memory>

LUWRA_NS_BEGIN

namespace internal {
	// Create reference to the value pointed to by `index`. Does not remove the referenced value.
	inline
	int referenceValue(State* state, int index) {
		lua_pushvalue(state, index);
		return luaL_ref(state, LUA_REGISTRYINDEX);
	}
}

/// Lifecyle of a reference
struct RefLifecycle {
	/// State with the reference registry
	State* state;

	/// Reference identification
	int ref;

	/// Create a reference using the value on top of the stack. Consumes the value.
	inline
	RefLifecycle(State* state): state(state), ref(luaL_ref(state, LUA_REGISTRYINDEX)) {}

	/// Create a reference to a value on the stack. Does not consume the value.
	inline
	RefLifecycle(State* state, int index): state(state) {
		lua_pushvalue(state, index);
		ref = luaL_ref(state, LUA_REGISTRYINDEX);
	}

	/// Create a reference using an existing one. The lifecycles of these references are
	/// independent.
	inline
	RefLifecycle(const RefLifecycle& other): state(other.state) {
		lua_rawgeti(other.state, LUA_REGISTRYINDEX, other.ref);
		ref = luaL_ref(state, LUA_REGISTRYINDEX);
	}

	/// Take over an existing reference. The given reference's lifecycle is terminated.
	inline
	RefLifecycle(RefLifecycle&& other): state(other.state), ref(other.ref) {
		other.ref = LUA_NOREF;
	}

	inline
	~RefLifecycle() {
		luaL_unref(state, LUA_REGISTRYINDEX, ref);
	}

	/// Push the value inside the reference cell onto the originating Lua stack.
	inline
	void load() const {
		lua_rawgeti(state, LUA_REGISTRYINDEX, ref);
	}

	/// Push the value inside the reference cell onto the given Lua stack.
	inline
	void load(State* target) const {
		lua_rawgeti(state, LUA_REGISTRYINDEX, ref);

		if (target != state)
			lua_xmove(state, target, 1);
	}
};

/// Handle for a reference
struct Reference {
	/// Smart pointer to the reference's lifecycle manager
	///
	/// Why a smart pointer? Copying RefLifecycle creates new Lua references, which we want to
	/// avoid. Therefore we use shared_ptr which gives us cheap reference counting.
	std::shared_ptr<const RefLifecycle> life;

	/// Create a reference using the value on top of the stack. Consumes the value.
	inline
	Reference(State* state):
		life(std::make_shared<RefLifecycle>(state))
	{}

	/// Create a reference to a value on the stack. Does not consume the value.
	inline
	Reference(State* state, int index):
		life(std::make_shared<RefLifecycle>(state, index))
	{}
};

/// Enables referencing/dereferencing values
template <>
struct Value<Reference> {
	static inline
	Reference read(State* state, int index) {
		return {state, index};
	}

	static inline
	void push(State* state, const Reference& value) {
		if (!value.life) {
			lua_pushnil(state);
			return;
		}

		value.life->load(state);
	}
};

LUWRA_NS_END

#endif

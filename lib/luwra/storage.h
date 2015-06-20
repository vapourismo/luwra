/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_STORAGE_H_
#define LUWRA_STORAGE_H_

#include "common.h"

LUWRA_NS_BEGIN

/**
 * A seperate execution stack, which acts as a storage unit.
 */
struct Storage {
	State* storage;

	/**
	 * Create the storage unit.
	 */
	inline
	Storage(State* parent, int capacity = 0) {
		storage = lua_newthread(parent);
		lua_pop(parent, 1);

		reserve(capacity);
	}

	/**
	 * Make sure at least `n` slots are available.
	 */
	inline
	void reserve(int n) {
		while (lua_gettop(storage) < n) {
			lua_pushnil(storage);
		}
	}

	/**
	 * Fill a slot.
	 */
	template <typename T> inline
	void set(int n, T value) {
		reserve(n);
		lua_remove(storage, n);
		Value<T>::push(storage, value);
		lua_insert(storage, n);
	}

	/**
	 * Implicit conversion to a Lua state.
	 */
	inline
	operator State*() {
		return storage;
	}
};

LUWRA_NS_END

#endif

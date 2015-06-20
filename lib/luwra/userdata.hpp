/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_USERDATA_H_
#define LUWRA_USERDATA_H_

#include "common.hpp"
#include "types.hpp"
#include "stack.hpp"

#include <sstream>
#include <utility>

LUWRA_NS_BEGIN

/**
 * Instances of userdata shall always be used as references, because other Lua types can not be
 * converted to references, hence this allows the compiler to differentiate between them.
 */
template <typename T>
struct Value<T&> {
	static inline
	T& read(State* state, int n) {
		return *static_cast<T*>(
			luaL_checkudata(state, n, T::MetatableName)
		);
	}

	template <typename... A> static inline
	int push(State* state, A&&... args) {
		void* mem = lua_newuserdata(state, sizeof(T));

		if (!mem) {
			luaL_error(state, "Failed to allocate user data");
			return -1;
		}

		// Call constructor on instance
		new (mem) T(std::forward<A>(args)...);

		// Set metatable for this type
		luaL_getmetatable(state, T::MetatableName);
		lua_setmetatable(state, -2);

		return 1;
	}
};

namespace internal {
	template <typename T, typename... A>
	int userdata_ctor(State* state) {
		return apply(state, std::function<int(A...)>([state](A... args) {
			return Value<T&>::push(state, args...);
		}));
	}

	template <typename T>
	int userdata_dtor(State* state) {
		Value<T&>::read(state, 1).~T();
		return 0;
	}

	template <typename T>
	int userdata_tostring(State* state) {
		return Value<std::string>::push(
			state,
			"TypeInstance" + std::string(T::MetatableName)
		);
	}
}

/**
 * Generate the metatable for the userdata type `T`. This function allows you to register methods
 * which are shared across all instances of this type. A garbage-collector hook is also inserted;
 * it destructs the underlying type when the garbage-collector says it is time to say good-bye
 * needed.
 */
template <typename T> static inline
void register_type_metatable(
	State* state,
	std::initializer_list<std::pair<const char*, CFunction>> methods,
	std::initializer_list<std::pair<const char*, CFunction>> meta_methods = {}
) {
	luaL_newmetatable(state, T::MetatableName);

	// Register methods
	lua_pushstring(state, "__index");
	lua_newtable(state);

	for (auto& method: methods) {
		lua_pushstring(state, method.first);
		lua_pushcfunction(state, method.second);
		lua_rawset(state, -3);
	}

	lua_rawset(state, -3);

	// Register garbage-collection hook
	lua_pushstring(state, "__gc");
	lua_pushcfunction(state, &internal::userdata_dtor<T>);
	lua_rawset(state, -3);

	// Register string representation function
	lua_pushstring(state, "__tostring");
	lua_pushcfunction(state, &internal::userdata_tostring<T>);
	lua_rawset(state, -3);

	// Insert meta methods
	for (auto& metamethod: meta_methods) {
		lua_pushstring(state, metamethod.first);
		lua_pushcfunction(state, metamethod.second);
		lua_rawset(state, -3);
	}

	// Pop meta table off the stack
	lua_pop(state, -1);
}

static inline
void register_userdata_metatable(
	State* state, const char* name,
	std::initializer_list<std::pair<const char*, CFunction>> methods,
	std::initializer_list<std::pair<const char*, CFunction>> meta_methods = {}
) {
	// Create meta table
	luaL_newmetatable(state, name);

	// Prepare for method registration
	lua_pushstring(state, "__index");
	lua_newtable(state);

	// Insert methods
	for (auto& method: methods) {
		lua_pushstring(state, method.first);
		lua_pushcfunction(state, method.second);
		lua_rawset(state, -3);
	}

	// Commit '__index' field
	lua_rawset(state, -3);

	// Insert meta methods
	for (auto& metamethod: meta_methods) {
		lua_pushstring(state, metamethod.first);
		lua_pushcfunction(state, metamethod.second);
		lua_rawset(state, -3);
	}

	// Pop meta table off the stack
	lua_pop(state, -1);
}

/**
 * Constructor function for a type `T`. Variadic arguments must be used to specify which parameters
 * to use during construction.
 */
template <typename T, typename... A>
constexpr CFunction WrapConstructor = &internal::userdata_ctor<T, A...>;

LUWRA_NS_END

#endif

/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_USERTYPES_H_
#define LUWRA_USERTYPES_H_

#include "common.hpp"
#include "types.hpp"
#include "stack.hpp"
#include "functions.hpp"

#include <map>
#include <utility>
#include <atomic>
#include <cassert>

LUWRA_NS_BEGIN

namespace internal {
	static
	std::atomic_size_t UserTypeCounter;

	template <typename T>
	std::string UserTypeName = "";

	template <typename T, typename... A>
	int UserTypeConstructor(State* state) {
		return internal::Layout<int(A...)>::Direct(
			state,
			1,
			&Value<T&>::template Push<A...>,
			state
		);
	}

	template <typename T>
	int UserTypeDestructor(State* state) {
		if (!lua_islightuserdata(state, 1))
			Value<T&>::Read(state, 1).~T();

		return 0;
	}

	template <typename T>
	int UserTypeToString(State* state) {
		return Value<std::string>::Push(
			state,
			internal::UserTypeName<T>
		);
	}

	template <typename T, typename R, R T::* PropertyPointer>
	int UserTypeAccessor(State* state) {
		if (lua_gettop(state) > 1) {
			// Setter
			(Value<T&>::Read(state, 1).*PropertyPointer) = Value<R>::Read(state, 2);
			return 0;
		} else {
			// Getter
			return Value<R>::Push(state, Value<T&>::Read(state, 1).*PropertyPointer);
		}
	}

	template <typename T, typename S>
	struct MethodWrapper {
		static_assert(
			sizeof(T) == -1,
			"The MethodWrapper template expects a type name and a function signature as parameter"
		);
	};

	template <typename T, typename R, typename... A>
	struct MethodWrapper<T, R(A...)> {
		using MethodPointerType = R (T::*)(A...);
		using FunctionSignature = R (T&, A...);

		template <MethodPointerType MethodPointer> static
		R Delegate(T& parent, A... args) {
			return (parent.*MethodPointer)(std::forward<A>(args)...);
		}
	};
}

/**
 * User type T.
 * Instances created using this specialization are allocated and constructed as full user data
 * types in Lua. The default garbage-collecting hook will destruct the user type, once it has
 * been marked.
 */
template <typename T>
struct Value<T&> {
	static inline
	T& Read(State* state, int n) {
		assert(!internal::UserTypeName<T>.empty());

		return *static_cast<T*>(
			luaL_checkudata(state, n, internal::UserTypeName<T>.c_str())
		);
	}

	template <typename... A> static inline
	int Push(State* state, A&&... args) {
		assert(!internal::UserTypeName<T>.empty());

		void* mem = lua_newuserdata(state, sizeof(T));

		if (!mem) {
			luaL_error(state, "Failed to allocate user type");
			return -1;
		}

		// Call constructor on instance
		new (mem) T(std::forward<A>(args)...);

		// Set metatable for this type
		luaL_getmetatable(state, internal::UserTypeName<T>.c_str());
		lua_setmetatable(state, -2);

		return 1;
	}
};

/**
 * User type T.
 * Instances created using this specialization are allocated as light user data in Lua.
 * The default garbage-collector does not destruct light user data types.
 */
template <typename T>
struct Value<T*> {
	static inline
	T* Read(State* state, int n) {
		assert(!internal::UserTypeName<T>.empty());

		return static_cast<T*>(
			luaL_checkudata(state, n, internal::UserTypeName<T>.c_str())
		);
	}

	static inline
	int Push(State* state, T* instance) {
		assert(!internal::UserTypeName<T>.empty());

		// Push instance as light user data
		lua_pushlightuserdata(state, instance);

		// Set metatable for this type
		luaL_getmetatable(state, internal::UserTypeName<T>.c_str());
		lua_setmetatable(state, -2);

		return 1;
	}
};

/**
 * Register the metatable for user type `T`. This function allows you to register methods
 * which are shared across all instances of this type. A garbage-collector hook is also inserted.
 * Meta-methods can be added and/or overwritten aswell.
 */
template <typename T> static inline
void RegisterUserType(
	State* state,
	const std::map<const char*, CFunction>& methods,
	const std::map<const char*, CFunction>& meta_methods = {}
) {
	// Setup an appropriate meta table name
	if (internal::UserTypeName<T>.empty())
		internal::UserTypeName<T> = "UD#" + std::to_string(internal::UserTypeCounter++);

	luaL_newmetatable(state, internal::UserTypeName<T>.c_str());

	// Register methods
	if (methods.size() > 0 && meta_methods.count("__index") == 0) {
		lua_pushstring(state, "__index");
		lua_newtable(state);

		for (auto& method: methods) {
			lua_pushstring(state, method.first);
			lua_pushcfunction(state, method.second);
			lua_rawset(state, -3);
		}

		lua_rawset(state, -3);
	}

	// Register garbage-collection hook
	if (meta_methods.count("__gc") == 0) {
		lua_pushstring(state, "__gc");
		lua_pushcfunction(state, &internal::UserTypeDestructor<T>);
		lua_rawset(state, -3);
	}

	// Register string representation function
	if (meta_methods.count("__tostring") == 0) {
		lua_pushstring(state, "__tostring");
		lua_pushcfunction(state, &internal::UserTypeToString<T>);
		lua_rawset(state, -3);
	}

	// Insert meta methods
	for (const auto& metamethod: meta_methods) {
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
constexpr CFunction WrapConstructor = &internal::UserTypeConstructor<T, A...>;

/**
 * Works similiar to `WrapFunction`. Given a class or struct declaration as follows:
 *
 *   struct T {
 *     R my_method(A0, A1 ... An);
 *   };
 *
 * You might wrap this method easily:
 *
 *   CFunction wrapped_meth = WrapMethod<T, R(A0, A1 ... An), &T::my_method>;
 *
 * In Lua, assuming `instance` is a userdata instance of type `T`, x0, x1 ... xn are instances
 * of A0, A1 ... An, and the method has been bound as `my_method`; it is possible to invoke the
 * method like so:
 *
 *   instance:my_method(x0, x1 ... xn)
 */
template <
	typename T,
	typename S,
	typename internal::MethodWrapper<T, S>::MethodPointerType MethodPointer
>
constexpr CFunction WrapMethod =
	WrapFunction<
		typename internal::MethodWrapper<T, S>::FunctionSignature,
		internal::MethodWrapper<T, S>::template Delegate<MethodPointer>
	>;

/**
 * Property accessor method
 *
 *   struct T {
 *     R my_property;
 *   };
 *
 * The wrapped property accessor is also a function:
 *
 *   CFunction wrapped_property = WrapProperty<T, R, &T::my_property>;
 */
template <
	typename T,
	typename R,
	R T::* PropertyPointer
>
constexpr CFunction WrapProperty = &internal::UserTypeAccessor<T, R, PropertyPointer>;

LUWRA_NS_END

#endif

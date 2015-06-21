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

LUWRA_NS_BEGIN

namespace internal {
	template <typename T, typename... A> inline
	int construct_user_type(State* state) {
		return internal::Layout<int(A...)>::direct(
			state,
			1,
			&Value<T&>::template push<A...>,
			state
		);
	}

	template <typename T> inline
	int destruct_user_type(State* state) {
		if (!lua_islightuserdata(state, 1))
			Value<T&>::read(state, 1).~T();

		return 0;
	}

	template <typename T>
	std::string user_type_identifier =
		"UD#" + std::to_string(uintmax_t(&destruct_user_type<T>));

	template <typename T>
	std::string stringify_user_type(T& val) {
		return
			internal::user_type_identifier<T>
			+ "@"
			+ std::to_string(uintmax_t(&val));
	}

	template <typename T, typename R, R T::* property_pointer> inline
	int access_user_type_property(State* state) {
		if (lua_gettop(state) > 1) {
			// Setter
			(Value<T&>::read(state, 1).*property_pointer) = Value<R>::read(state, 2);
			return 0;
		} else {
			// Getter
			return push(state, Value<T&>::read(state, 1).*property_pointer);
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

		template <MethodPointerType method_pointer> static inline
		R call(T& parent, A... args) {
			return (parent.*method_pointer)(std::forward<A>(args)...);
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
	T& read(State* state, int n) {
		return *static_cast<T*>(
			luaL_checkudata(state, n, internal::user_type_identifier<T>.c_str())
		);
	}

	template <typename... A> static inline
	int push(State* state, A&&... args) {
		void* mem = lua_newuserdata(state, sizeof(T));

		if (!mem) {
			luaL_error(state, "Failed to allocate user type");
			return -1;
		}

		// Call constructor on instance
		new (mem) T(std::forward<A>(args)...);

		// Set metatable for this type
		luaL_getmetatable(state, internal::user_type_identifier<T>.c_str());
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
	T* read(State* state, int n) {
		return static_cast<T*>(
			luaL_checkudata(state, n, internal::user_type_identifier<T>.c_str())
		);
	}

	static inline
	int push(State* state, T* instance) {
		// push instance as light user data
		lua_pushlightuserdata(state, instance);

		// Set metatable for this type
		luaL_getmetatable(state, internal::user_type_identifier<T>.c_str());
		lua_setmetatable(state, -2);

		return 1;
	}
};

/**
 * Constructor function for a type `T`. Variadic arguments must be used to specify which parameters
 * to use during construction.
 */
template <typename T, typename... A>
constexpr CFunction wrap_constructor = &internal::construct_user_type<T, A...>;

/**
 * Works similiar to `wrap_function`. Given a class or struct declaration as follows:
 *
 *   struct T {
 *     R my_method(A0, A1 ... An);
 *   };
 *
 * You might wrap this method easily:
 *
 *   CFunction wrapped_meth = wrap_method<T, R(A0, A1 ... An), &T::my_method>;
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
	typename internal::MethodWrapper<T, S>::MethodPointerType method_pointer
>
constexpr CFunction wrap_method =
	wrap_function<
		typename internal::MethodWrapper<T, S>::FunctionSignature,
		internal::MethodWrapper<T, S>::template call<method_pointer>
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
 *   CFunction wrapped_property = wrap_property<T, R, &T::my_property>;
 */
template <
	typename T,
	typename R,
	R T::* property_pointer
>
constexpr CFunction wrap_property =
	&internal::access_user_type_property<T, R, property_pointer>;

/**
 * Register the metatable for user type `T`. This function allows you to register methods
 * which are shared across all instances of this type. A garbage-collector hook is also inserted.
 * Meta-methods can be added and/or overwritten aswell.
 */
template <typename T> static inline
void register_user_type(
	State* state,
	const std::map<const char*, CFunction>& methods,
	const std::map<const char*, CFunction>& meta_methods = {}
) {
	// Setup an appropriate meta table name
	luaL_newmetatable(state, internal::user_type_identifier<T>.c_str());

	// Register methods
	if (methods.size() > 0 && meta_methods.count("__index") == 0) {
		push(state, "__index");
		lua_newtable(state);

		for (auto& method: methods) {
			push(state, method.first);
			push(state, method.second);
			lua_rawset(state, -3);
		}

		lua_rawset(state, -3);
	}

	// Register garbage-collection hook
	if (meta_methods.count("__gc") == 0) {
		push(state, "__gc");
		push(state, &internal::destruct_user_type<T>);
		lua_rawset(state, -3);
	}

	// Register string representation function
	if (meta_methods.count("__tostring") == 0) {
		push(state, "__tostring");
		push(state, wrap_function<std::string(T&), &internal::stringify_user_type<T>>);
		lua_rawset(state, -3);
	}

	// Insert meta methods
	for (const auto& metamethod: meta_methods) {
		push(state, metamethod.first);
		push(state, metamethod.second);
		lua_rawset(state, -3);
	}

	// Pop meta table off the stack
	lua_pop(state, -1);
}

LUWRA_NS_END

#endif

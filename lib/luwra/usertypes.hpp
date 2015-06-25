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
	using UserTypeID = const void*;

	template <typename T>
	using StripUserType = std::remove_cv_t<T>;

	/**
	 * User type identifier
	 */
	template <typename T>
	UserTypeID user_type_id = (void*) INTPTR_MAX;

	/**
	 * Registry name for a meta table which is associated with a user type
	 */
	template <typename T>
	std::string user_type_reg_name =
		"UD#" + std::to_string(uintptr_t(&user_type_id<T>));

	/**
	 * Register a new meta table for a user type T.
	 */
	template <typename U> static inline
	void new_user_type_id(State* state) {
		using T = StripUserType<U>;

		// Use the address as user type identifier
		luaL_newmetatable(state, user_type_reg_name<T>.c_str());
		user_type_id<T> = lua_topointer(state, -1);
	}

	/**
	 * Get the identifier for a user type at the given index.
	 */
	static inline
	UserTypeID get_user_type_id(State* state, int index) {
		if (!lua_isuserdata(state, index))
			return nullptr;

		if (lua_getmetatable(state, index)) {
			UserTypeID type_id = lua_topointer(state, -1);
			lua_pop(state, 1);
			return type_id;
		} else {
			return nullptr;
		}
	}

	/**
	 * Check if the value at the given index if a user type T.
	 */
	template <typename U> static inline
	StripUserType<U>* check_user_type(State* state, int index) {
		using T = StripUserType<U>;

		if (get_user_type_id(state, index) == user_type_id<T>) {
			return static_cast<T*>(lua_touserdata(state, index));
		} else {
			std::string error_msg =
				"Expected user type " + std::to_string(uintptr_t(user_type_id<T>));
			luaL_argerror(state, index, error_msg.c_str());
			return nullptr;
		}
	}

	template <typename U> static inline
	void apply_user_type_meta_table(State* state) {
		luaL_getmetatable(state, user_type_reg_name<StripUserType<U>>.c_str());
		lua_setmetatable(state, -2);
	}

	/**
	 * Lua C function to construct a user type T with parameters A
	 */
	template <typename T, typename... A> static inline
	int construct_user_type(State* state) {
		return internal::Layout<int(A...)>::direct(
			state,
			1,
			&Value<T&>::template push<A...>,
			state
		);
	}

	/**
	 * Lua C function to destruct a user type T
	 */
	template <typename T> static inline
	int destruct_user_type(State* state) {
		if (!lua_islightuserdata(state, 1))
			Value<T&>::read(state, 1).~T();

		return 0;
	}

	/**
	 * Create a string representation for user type T.
	 */
	template <typename U> static
	int stringify_user_type(State* state) {
		return Value<std::string>::push(
			state,
			internal::user_type_reg_name<StripUserType<U>>
			+ "@"
			+ std::to_string(uintptr_t(Value<U*>::read(state, 1)))
		);
	}

	/**
	 * Lua C function for a property accessor.
	 */
	template <typename T, typename R, R T::* property_pointer> static inline
	int access_user_type_property(State* state) {
		if (lua_gettop(state) > 1) {
			// Setter
			(Value<T*>::read(state, 1)->*property_pointer) = Value<R>::read(state, 2);
			return 0;
		} else {
			// Getter
			return push(state, Value<T*>::read(state, 1)->*property_pointer);
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
		using FunctionSignature = R (T*, A...);

		/**
		 * This function is a wrapped around the invocation of a given method.
		 */
		template <MethodPointerType method_pointer> static inline
		R call(T* parent, A... args) {
			return (parent->*method_pointer)(std::forward<A>(args)...);
		}
	};
}

/**
 * User type T.
 * Instances created using this specialization are allocated and constructed as full user data
 * types in Lua. The default garbage-collecting hook will destruct the user type, once it has
 * been marked.
 */
template <typename U>
struct Value<U&> {
	using T = internal::StripUserType<U>;

	static inline
	T& read(State* state, int n) {
		return *internal::check_user_type<T>(state, n);
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
		internal::apply_user_type_meta_table<T>(state);

		return 1;
	}
};

/**
 * User type T.
 * Instances created using this specialization are allocated as light user data in Lua.
 * The default garbage-collector does not destruct light user data types.
 */
template <typename U>
struct Value<U*> {
	using T = internal::StripUserType<U>;

	static inline
	T* read(State* state, int n) {
		return internal::check_user_type<T>(state, n);
	}

	static inline
	int push(State* state, T* instance) {
		if (instance == nullptr)
			return 0;

		// push instance as light user data
		lua_pushlightuserdata(state, instance);

		// Set metatable for this type
		internal::apply_user_type_meta_table<T>(state);

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
	const std::map<const char*, CFunction>& meta_methods = std::map<const char*, CFunction>()
) {
	// Setup an appropriate meta table name
	internal::new_user_type_id<T>(state);

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
		push(state, &internal::stringify_user_type<T>);
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

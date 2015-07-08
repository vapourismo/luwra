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
	template <typename T> extern
	const UserTypeID user_type_id = (void*) INTPTR_MAX;

	/**
	 * Registry name for a metatable which is associated with a user type
	 */
	template <typename T> extern
	const std::string user_type_reg_name =
		"UD#" + std::to_string(uintptr_t(&user_type_id<StripUserType<T>>));

	/**
	 * Register a new metatable for a user type T.
	 */
	template <typename U> static inline
	void new_user_type_metatable(State* state) {
		using T = StripUserType<U>;
		luaL_newmetatable(state, user_type_reg_name<T>.c_str());
	}

	/**
	 * Check if the value at the given index if a user type T.
	 */
	template <typename U> static inline
	StripUserType<U>* check_user_type(State* state, int index) {
		using T = StripUserType<U>;

		return static_cast<T*>(luaL_checkudata(state, index, user_type_reg_name<T>.c_str()));
	}

	/**
	 * Apply U's metatable for the value at the top of the stack.
	 */
	template <typename U> static inline
	void apply_user_type_meta_table(State* state) {
		using T = StripUserType<U>;

		luaL_getmetatable(state, user_type_reg_name<T>.c_str());
		lua_setmetatable(state, -2);
	}

	/**
	 * Lua C function to construct a user type T with parameters A
	 */
	template <typename U, typename... A> static inline
	int construct_user_type(State* state) {
		return internal::Layout<int(A...)>::direct(
			state,
			1,
			&Value<StripUserType<U>&>::template push<A...>,
			state
		);
	}

	/**
	 * Lua C function to destruct a user type T
	 */
	template <typename U> static inline
	int destruct_user_type(State* state) {
		using T = StripUserType<U>;

		if (!lua_islightuserdata(state, 1))
			Value<T&>::read(state, 1).~T();

		return 0;
	}

	/**
	 * Create a string representation for user type T.
	 */
	template <typename U> static
	int stringify_user_type(State* state) {
		using T = StripUserType<U>;

		return Value<std::string>::push(
			state,
			internal::user_type_reg_name<T>
				+ "@"
				+ std::to_string(uintptr_t(Value<T*>::read(state, 1)))
		);
	}

	/**
	 * Helper struct for wrapping user type fields
	 */
	template <typename U, typename R>
	struct FieldWrapper {
		using T = StripUserType<U>;

		template <R T::* field_pointer> static inline
		int invoke(State* state) {
			if (lua_gettop(state) > 1) {
				// Setter
				Value<T*>::read(state, 1)->*field_pointer = Value<R>::read(state, 2);
				return 0;
			} else {
				// Getter
				return push(state, Value<T*>::read(state, 1)->*field_pointer);
			}
		}
	};

	// 'const'-qualified fields
	template <typename U, typename R>
	struct FieldWrapper<U, const R> {
		using T = StripUserType<U>;

		template <const R T::* field_pointer> static inline
		int invoke(State* state) {
			return push(state, Value<T*>::read(state, 1)->*field_pointer);
		}
	};

	template <typename T>
	struct FieldWrapperHelper {
		static_assert(
			sizeof(T) == -1,
			"Parameter to FieldWrapperHelper is not a function pointer"
		);
	};

	template <typename T, typename R>
	struct FieldWrapperHelper<R T::*>: FieldWrapper<T, R> {};

	/**
	 * Helper struct for wrapping user type methods
	 */
	template <typename T, typename S>
	struct MethodWrapper {
		static_assert(
			sizeof(T) == -1,
			"Undefined template MethodWrapper"
		);
	};

	// 'const volatile'-qualified methods
	template <typename T, typename R, typename... A>
	struct MethodWrapper<const volatile T, R(A...)> {
		using MethodPointerType = R (T::*)(A...) const volatile;
		using FunctionSignature = R (const volatile T*, A...);

		template <MethodPointerType method_pointer> static inline
		R call(const volatile T* parent, A... args) {
			return (parent->*method_pointer)(std::forward<A>(args)...);
		}

		template <MethodPointerType method_pointer> static inline
		int invoke(State* state) {
			return FunctionWrapper<FunctionSignature>::template invoke<call<method_pointer>>(state);
		}
	};

	// 'const'-qualified methods
	template <typename T, typename R, typename... A>
	struct MethodWrapper<const T, R(A...)> {
		using MethodPointerType = R (T::*)(A...) const;
		using FunctionSignature = R (const T*, A...);

		template <MethodPointerType method_pointer> static inline
		R call(const T* parent, A... args) {
			return (parent->*method_pointer)(std::forward<A>(args)...);
		}

		template <MethodPointerType method_pointer> static inline
		int invoke(State* state) {
			return FunctionWrapper<FunctionSignature>::template invoke<call<method_pointer>>(state);
		}
	};

	// 'volatile'-qualified methods
	template <typename T, typename R, typename... A>
	struct MethodWrapper<volatile T, R(A...)> {
		using MethodPointerType = R (T::*)(A...) volatile;
		using FunctionSignature = R (volatile T*, A...);

		template <MethodPointerType method_pointer> static inline
		R call(volatile T* parent, A... args) {
			return (parent->*method_pointer)(std::forward<A>(args)...);
		}

		template <MethodPointerType method_pointer> static inline
		int invoke(State* state) {
			return FunctionWrapper<FunctionSignature>::template invoke<call<method_pointer>>(state);
		}
	};

	// unqualified methods
	template <typename T, typename R, typename... A>
	struct MethodWrapper<T, R(A...)> {
		using MethodPointerType = R (T::*)(A...);
		using FunctionSignature = R (T*, A...);

		template <MethodPointerType method_pointer> static inline
		R call(T* parent, A... args) {
			return (parent->*method_pointer)(std::forward<A>(args)...);
		}

		template <MethodPointerType method_pointer> static inline
		int invoke(State* state) {
			return FunctionWrapper<FunctionSignature>::template invoke<call<method_pointer>>(state);
		}
	};

	template <typename T>
	struct MethodWrapperHelper {
		static_assert(
			sizeof(T) == -1,
			"Parameter to MethodWrapperHelper is not a function pointer"
		);
	};

	template <typename T, typename R, typename... A>
	struct MethodWrapperHelper<R (T::*)(A...) const volatile>:
		MethodWrapper<const volatile T, R(A...)>
	{};

	template <typename T, typename R, typename... A>
	struct MethodWrapperHelper<R (T::*)(A...) const>:
		MethodWrapper<const T, R(A...)>
	{};

	template <typename T, typename R, typename... A>
	struct MethodWrapperHelper<R (T::*)(A...) volatile>:
		MethodWrapper<volatile T, R(A...)>
	{};

	template <typename T, typename R, typename... A>
	struct MethodWrapperHelper<R (T::*)(A...)>:
		MethodWrapper<T, R(A...)>
	{};
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
	U& read(State* state, int n) {
		// T is unqualified, therefore conversion from T& to U& is allowed
		return *internal::check_user_type<T>(state, n);
	}

	template <typename... A> static inline
	int push(State* state, A&&... args) {
		void* mem = lua_newuserdata(state, sizeof(T));

		if (!mem) {
			luaL_error(state, "Failed to allocate user type");
			return -1;
		}

		// Construct
		new (mem) T {std::forward<A>(args)...};

		// Apply metatable for unqualified type T
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
	U* read(State* state, int n) {
		// T is unqualified, therefore conversion from T* to U* is allowed
		return internal::check_user_type<T>(state, n);
	}

	static inline
	int push(State* state, T* instance) {
		if (instance == nullptr)
			return 0;

		// Push instance as light user data
		lua_pushlightuserdata(state, instance);

		// Apply metatable for unqualified type T
		internal::apply_user_type_meta_table<T>(state);

		return 1;
	}
};

/**
 * Constructor function for a type `T`. Variadic arguments must be used to specify which parameters
 * to use during construction.
 */
template <typename T, typename... A>
constexpr CFunction wrap_constructor =
	&internal::construct_user_type<internal::StripUserType<T>, A...>;

/**
 * This macros has no additional use whatsoever, but I makes the style consistent.
 */
#define LUWRA_WRAP_CONSTRUCTOR(type, ...) \
	(luwra::wrap_constructor<type, __VA_ARGS__>)

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
 * This macro allows you to wrap methods without supplying template parameters.
 */
#define LUWRA_WRAP_METHOD(meth) \
	(&luwra::internal::MethodWrapperHelper<decltype(&meth)>::template invoke<&meth>)

/**
 * Property accessor method
 *
 *   struct T {
 *     R my_property;
 *   };
 *
 * The wrapped property accessor is also a function:
 *
 *   CFunction wrapped_property = wrap_field<T, R, &T::my_property>;
 */
template <
	typename T,
	typename R,
	R T::* field_pointer
>
constexpr CFunction wrap_field =
	&internal::FieldWrapper<T, R>::template invoke<field_pointer>;

/**
 * This macro allows you to wrap fields without supplying template parameters.
 */
#define LUWRA_WRAP_FIELD(field) \
	(&luwra::internal::FieldWrapperHelper<decltype(&field)>::invoke<&field>)

/**
 * Register the metatable for user type `T`. This function allows you to register methods
 * which are shared across all instances of this type. A garbage-collector hook is also inserted.
 * Meta-methods can be added and/or overwritten aswell.
 */
template <typename U> static inline
void register_user_type(
	State* state,
	const std::map<const char*, CFunction>& methods = std::map<const char*, CFunction>(),
	const std::map<const char*, CFunction>& meta_methods = std::map<const char*, CFunction>()
) {
	using T = internal::StripUserType<U>;

	// Setup an appropriate metatable name
	internal::new_user_type_metatable<T>(state);

	// Register methods
	if (methods.size() > 0 && meta_methods.count("__index") == 0) {
		push(state, "__index");
		lua_newtable(state);

		for (auto& method: methods) {
			set_fields(state, -1, method.first, method.second);
		}

		lua_rawset(state, -3);
	}

	// Register garbage-collection hook
	if (meta_methods.count("__gc") == 0) {
		set_fields(state, -1, "__gc", &internal::destruct_user_type<T>);
	}

	// Register string representation function
	if (meta_methods.count("__tostring") == 0) {
		set_fields(state, -1, "__tostring", &internal::stringify_user_type<T>);
	}

	// Insert meta methods
	for (const auto& metamethod: meta_methods) {
		set_fields(state, -1, metamethod.first, metamethod.second);
	}

	// Pop metatable off the stack
	lua_pop(state, -1);
}

LUWRA_NS_END

#endif

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

#include <map>
#include <string>

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
		return direct<int(A...)>(
			state,
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
			read<T&>(state, 1).~T();

		return 0;
	}

	/**
	 * Create a string representation for user type T.
	 */
	template <typename U> static
	int stringify_user_type(State* state) {
		using T = StripUserType<U>;

		return push(
			state,
			internal::user_type_reg_name<T>
				+ "@"
				+ std::to_string(uintptr_t(Value<T*>::read(state, 1)))
		);
	}
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
 * Register the metatable for user type `T`. This function allows you to register methods
 * which are shared across all instances of this type.
 *
 * By default a garbage-collector hook and string representation function are added as meta methods.
 * Both can be overwritten.
 *
 * \tparam U User type struct or class
 *
 * \param state        Lua state
 * \param methods      Map of methods
 * \param meta_methods Map of meta methods
 */
template <typename U> static inline
void registerUserType(
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
			setFields(state, -1, method.first, method.second);
		}

		lua_rawset(state, -3);
	}

	// Register garbage-collection hook
	if (meta_methods.count("__gc") == 0) {
		setFields(state, -1, "__gc", &internal::destruct_user_type<T>);
	}

	// Register string representation function
	if (meta_methods.count("__tostring") == 0) {
		setFields(state, -1, "__tostring", &internal::stringify_user_type<T>);
	}

	// Insert meta methods
	for (const auto& metamethod: meta_methods) {
		setFields(state, -1, metamethod.first, metamethod.second);
	}

	// Pop metatable off the stack
	lua_pop(state, -1);
}

namespace internal {
	template <typename T>
	struct UserTypeSignature {
		static_assert(
			sizeof(T) == -1,
			"Parameter to UserTypeSignature is not a valid signature"
		);
	};

	template <typename T, typename... A>
	struct UserTypeSignature<T(A...)> {
		using UserType = T;

		static inline
		void registerConstructor(State* state, const std::string& name) {
			setGlobal(state, name, &construct_user_type<UserType, A...>);
		}
	};
}

/**
 * Same as the other `registerUserType` but registers the construtor as well. The template parameter
 * is a signature `U(A...)` where `U` is the user type and `A...` its constructor parameters.
 */
template <typename T> static inline
void registerUserType(
	State* state,
	const std::string& ctor_name,
	const std::map<const char*, CFunction>& methods = std::map<const char*, CFunction>(),
	const std::map<const char*, CFunction>& meta_methods = std::map<const char*, CFunction>()
) {
	using U = typename internal::UserTypeSignature<T>::UserType;
	registerUserType<U>(state, methods, meta_methods);
	internal::UserTypeSignature<T>::registerConstructor(state, ctor_name);
}

LUWRA_NS_END

/**
 * Generate a `lua_CFunction` wrapper for a constructor.
 * \param type Type to instantiate
 * \param ...  Constructor parameter types
 * \return Wrapped function as `lua_CFunction`
 */
#define LUWRA_WRAP_CONSTRUCTOR(type, ...) \
	(&luwra::internal::construct_user_type<luwra::internal::StripUserType<type>, __VA_ARGS__>)

#define LUWRA_FIELD(type, name) {__STRING(name), LUWRA_WRAP_FIELD(type::name)}
#define LUWRA_METHOD(type, name) {__STRING(name), LUWRA_WRAP_METHOD(type::name)}
#define LUWRA_FUNCTION(type, name) {__STRING(name), LUWRA_WRAP_FUNCTION(type::name)}

#endif

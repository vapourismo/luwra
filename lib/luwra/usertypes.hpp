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
	template <typename T>
	using StripUserType = typename std::remove_cv<T>::type;

	/**
	 * User type registry identifiers
	 */
	template <typename T>
	struct UserTypeReg {
		/**
		 * Dummy field which is used because it has a seperate address for each instance of T
		 */
		static
		const int id;

		/**
		 * Registry name for a metatable which is associated with a user type
		 */
		static
		const std::string name;
	};

	template <typename T>
	const int UserTypeReg<T>::id = INT_MAX;

	template <typename T>
	const std::string UserTypeReg<T>::name = "UD#" + std::to_string(uintptr_t(&id));

	/**
	 * Register a new metatable for a user type T.
	 */
	template <typename U> static inline
	void new_user_type_metatable(State* state) {
		using T = StripUserType<U>;
		luaL_newmetatable(state, UserTypeReg<T>::name.c_str());
	}

	/**
	 * Check if the value at the given index if a user type T.
	 */
	template <typename U> static inline
	StripUserType<U>* check_user_type(State* state, int index) {
		using T = StripUserType<U>;
		return static_cast<T*>(luaL_checkudata(state, index, UserTypeReg<T>::name.c_str()));
	}

	/**
	 * Apply U's metatable for the value at the top of the stack.
	 */
	template <typename U> static inline
	void apply_user_type_meta_table(State* state) {
		setMetatable(state, UserTypeReg<StripUserType<U>>::name.c_str());
	}

	/**
	 * Lua C function to construct a user type T with parameters A
	 */
	template <typename U, typename... A> static inline
	int construct_user_type(State* state) {
		return direct<size_t (A...)>(
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
			internal::UserTypeReg<T>::name
				+ "@"
				+ std::to_string(uintptr_t(Value<T*>::read(state, 1)))
		);
	}
}

/**
 * User type
 */
template <typename U>
struct Value<U&> {
	using T = internal::StripUserType<U>;

	/**
	 * Reference a user type value on the stack.
	 * \param state Lua state
	 * \param n     Stack index
	 * \returns Reference to the user type value
	 */
	static inline
	U& read(State* state, int n) {
		// T is unqualified, therefore conversion from T& to U& is allowed
		return *internal::check_user_type<T>(state, n);
	}

	/**
	 * Construct a user type value on the stack.
	 * \note Instances created using this specialization are allocated and constructed as full user
	 *       data types in Lua. The default garbage-collecting hook will destruct the user type,
	 *       once it has been marked.
	 * \param state Lua state
	 * \param args  Constructor arguments
	 * \returns Number of values that have been pushed onto the stack
	 */
	template <typename... A> static inline
	size_t push(State* state, A&&... args) {
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
 * User type
 */
template <typename U>
struct Value<U*> {
	using T = internal::StripUserType<U>;

	/**
	 * Reference a user type value on the stack.
	 * \param state Lua state
	 * \param n     Stack index
	 * \returns Pointer to the user type value.
	 */
	static inline
	U* read(State* state, int n) {
		// T is unqualified, therefore conversion from T* to U* is allowed
		return internal::check_user_type<T>(state, n);
	}

	/**
	 * Copy a value onto the stack. This function behaves exactly as if you would call
	 * `Value<U&>::push(state, *ptr)`.
	 * \param state Lua state
	 * \param ptr   Pointer to the user type value
	 * \returns Number of values that have been pushed
	 */
	static
	size_t push(State* state, const T* ptr) {
		return Value<T&>::push(state, *ptr);
	}
};

using MemberMap = std::map<std::string, CFunction>;

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
	const MemberMap& methods = MemberMap(),
	const MemberMap& meta_methods = MemberMap()
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
	struct UserTypeSignature<T (A...)> {
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
	const MemberMap& methods = MemberMap(),
	const MemberMap& meta_methods = MemberMap()
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

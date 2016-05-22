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
#include "auxiliary.hpp"

#include <map>
#include <string>

LUWRA_NS_BEGIN

namespace internal {
	template <typename T>
	using StripUserType = typename std::remove_cv<T>::type;

	// User type registry identifiers
	template <typename T>
	struct UserTypeReg {
		// Dummy field which is used because it has a seperate address for each instance of T
		static
		const int id;

		// Registry name for a metatable which is associated with a user type
		static
		const std::string name;
	};

	template <typename T>
	const int UserTypeReg<T>::id = INT_MAX;

	#ifndef LUWRA_REGISTRY_PREFIX
		#define LUWRA_REGISTRY_PREFIX "Luwra#"
	#endif

	template <typename T>
	const std::string UserTypeReg<T>::name =
		LUWRA_REGISTRY_PREFIX + std::to_string(uintptr_t(&id));

	template <typename U>
	struct UserTypeWrapper {
		using T = StripUserType<U>;
		using Reg = UserTypeReg<T>;

		// Read the userdata instance of T from the stack.
		static inline
		T* check(State* state, int index) {
			return static_cast<T*>(
				luaL_checkudata(state, index, Reg::name.c_str())
			);
		}

		// Constructs the user type in Lua.
		template <typename... A> static inline
		int construct(State* state) {
			return static_cast<int>(
				direct<size_t(A...)>(
					state,
					&Value<T&>::template push<A...>,
					state
				)
			);
		}

		// Use this as garbage-collector hook ('__gc' metatable); it will call the destructor.
		static inline
		int destruct(State* state) {
			(*check(state, 1)).~T();
			return 0;
		}

		// This is converts the userdata in Lua to a string.
		static inline
		int stringify(State* state) {
			return static_cast<int>(
				push(
					state,
					Reg::name + "@" + std::to_string(uintptr_t(check(state, 1)))
				)
			);
		}
	};
}

/**
 * Construct a user type value on the stack.
 * \note Instances created using this specialization are allocated and constructed as full user
 *       data types in Lua. The default garbage-collecting hook will destruct the user type,
 *       once it has been marked.
 * \param state Lua state
 * \param args  Constructor arguments
 * \returns Reference to the constructed value
 */
template <typename U, typename... A> static inline
internal::StripUserType<U>& construct(State* state, A&&... args) {
	using Wrapper = internal::UserTypeWrapper<U>;
	using T = typename Wrapper::T;

	void* mem = lua_newuserdata(state, sizeof(T));

	if (!mem) {
		luaL_error(state, "Failed to allocate user type");
		// 'luaL_error' will not return
	}

	// Construct
	T* value = new (mem) T {std::forward<A>(args)...};

	// Apply metatable for unqualified type
	setMetatable(state, Wrapper::Reg::name.c_str());

	return *value;
}

/**
 * User type
 */
template <typename U>
struct Value {
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
		return *internal::UserTypeWrapper<T>::check(state, n);
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
		construct<T>(state, std::forward<A>(args)...);
		return 1;
	}
};

template <typename U>
struct Value<U&>: Value<U> {};

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
		return internal::UserTypeWrapper<T>::check(state, n);
	}

	/**
	 * Copy a value onto the stack. This function behaves exactly as if you would call
	 * `Value<U&>::push(state, *ptr)`.
	 * \param state Lua state
	 * \param ptr   Pointer to the user type value
	 * \returns Number of values that have been pushed
	 */
	static inline
	size_t push(State* state, const T* ptr) {
		return Value<T&>::push(state, *ptr);
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
	const MemberMap& methods = MemberMap(),
	const MemberMap& meta_methods = MemberMap()
) {
	using Wrapper = internal::UserTypeWrapper<U>;

	// Setup an appropriate metatable name
	luaL_newmetatable(state, Wrapper::Reg::name.c_str());

	// Insert methods
	setFields(state, -1,
		"__index",    methods,
		"__gc",       &Wrapper::destruct,
		"__tostring", &Wrapper::stringify
	);

	// Insert meta methods
	setFields(state, -1, meta_methods);

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

	template <typename U, typename... A>
	struct UserTypeSignature<U(A...)> {
		using T = StripUserType<U>;

		static inline
		void registerConstructor(State* state, const char* name) {
			setGlobal(state, name, &UserTypeWrapper<T>::template construct<A...>);
		}
	};
}

/**
 * Same as the other `registerUserType` but registers the constructor as well. The template parameter
 * is a signature `U(A...)` where `U` is the user type and `A...` its constructor parameters.
 */
template <typename S> static inline
void registerUserType(
	State* state,
	const char* ctor_name,
	const MemberMap& methods = MemberMap(),
	const MemberMap& meta_methods = MemberMap()
) {
	using T = typename internal::UserTypeSignature<S>::T;
	registerUserType<T>(state, methods, meta_methods);
	internal::UserTypeSignature<S>::registerConstructor(state, ctor_name);
}

LUWRA_NS_END

/**
 * Generate a user type member manifest. This is basically any type which can be constructed using a
 * string and a `lua_CFunction`. For example `std::pair<Pushable, Pushable>`.
 */
#define LUWRA_MEMBER(type, name) \
	{#name, LUWRA_WRAP(__LUWRA_NS_RESOLVE(type, name))}

/**
 * Generate a `lua_CFunction` wrapper for a constructor.
 * \param type Type to instantiate
 * \param ...  Constructor parameter types
 * \return Wrapped function as `lua_CFunction`
 */
#define LUWRA_WRAP_CONSTRUCTOR(type, ...) \
	(&luwra::internal::UserTypeWrapper<luwra::internal::StripUserType<type>>::template construct<__VA_ARGS__>)

/**
 * Define the registry name for a user type.
 * \param type    User type
 * \param regname Registry name
 */
#define LUWRA_DEF_REGISTRY_NAME(type, regname) \
	LUWRA_NS_BEGIN \
	namespace internal { \
		template <> struct UserTypeReg<type> { static const std::string name; }; \
		const std::string UserTypeReg<type>::name = (regname); \
	} \
	LUWRA_NS_END

#endif

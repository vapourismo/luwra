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
	template <typename UserType>
	using StripUserType = typename std::remove_cv<UserType>::type;

	// User type registry identifiers
	template <typename UserType>
	struct UserTypeReg {
		// Dummy field which is used because it has a seperate address for each instance of UserType
		static
		const int id;

		// Registry name for a metatable which is associated with a user type
		static
		const std::string name;
	};

	template <typename UserType>
	const int UserTypeReg<UserType>::id = INT_MAX;

	#ifndef LUWRA_REGISTRY_PREFIX
		#define LUWRA_REGISTRY_PREFIX "Luwra#"
	#endif

	template <typename UserType>
	const std::string UserTypeReg<UserType>::name =
		LUWRA_REGISTRY_PREFIX + std::to_string(uintptr_t(&id));

	template <typename UserType>
	struct UserTypeWrapper: UserTypeReg<StripUserType<UserType>> {
		using Type = StripUserType<UserType>;
		using Reg = UserTypeReg<Type>;

		// Read the userdata instance of Type from the stack.
		static inline
		Type* check(State* state, int index) {
			return static_cast<Type*>(
				luaL_checkudata(state, index, Reg::name.c_str())
			);
		}

		// Use this as garbage-collector hook ('__gc' metatable); it will call the destructor.
		static inline
		int destruct(State* state) {
			(*check(state, 1)).~Type();
			return 0;
		}

		// This converts the userdata in Lua to a string.
		static inline
		int stringify(State* state) {
			return static_cast<int>(
				push(
					state,
					Reg::name + "@" + std::to_string(uintptr_t(check(state, 1)))
				)
			);
		}

		template <typename... Args>
		struct ConstructorWrapper {
			static inline
			int invoke(State* state) {
				return static_cast<int>(
					direct<size_t(Args...)>(
						state,
						&Value<Type>::template push<Args...>,
						state
					)
				);
			}
		};
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
template <typename UserType, typename... Args> static inline
internal::StripUserType<UserType>& construct(State* state, Args&&... args) {
	using Wrapper = internal::UserTypeWrapper<UserType>;
	using Type = typename Wrapper::Type;

	void* mem = lua_newuserdata(state, sizeof(Type));

	if (!mem) {
		luaL_error(state, "Failed to allocate user type");
		// 'luaL_error' will not return
	}

	// Construct
	Type* value = new (mem) Type {std::forward<Args>(args)...};

	// Apply metatable for unqualified type
	setMetatable(state, Wrapper::name.c_str());

	return *value;
}

/**
 * User type
 */
template <typename UserType>
struct Value {
	using Type = internal::StripUserType<UserType>;

	/**
	 * Reference a user type value on the stack.
	 * \param state Lua state
	 * \param n     Stack index
	 * \returns Reference to the user type value
	 */
	static inline
	UserType& read(State* state, int n) {
		// Type is unqualified, therefore conversion from Type& to UserType& is allowed
		return *internal::UserTypeWrapper<Type>::check(state, n);
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
	template <typename... Args> static inline
	size_t push(State* state, Args&&... args) {
		construct<Type>(state, std::forward<Args>(args)...);
		return 1;
	}
};

/**
 * User type
 */
template <typename UserType>
struct Value<UserType*> {
	using Type = internal::StripUserType<UserType>;

	/**
	 * Reference a user type value on the stack.
	 * \param state Lua state
	 * \param n     Stack index
	 * \returns Pointer to the user type value.
	 */
	static inline
	UserType* read(State* state, int n) {
		// Type is unqualified, therefore conversion from Type* to UserType* is allowed
		return internal::UserTypeWrapper<Type>::check(state, n);
	}

	/**
	 * Copy a value onto the stack. This function behaves exactly as if you would call
	 * `Value<UserType>::push(state, *ptr)`.
	 * \param state Lua state
	 * \param ptr   Pointer to the value
	 * \returns Number of values that have been pushed
	 */
	static inline
	size_t push(State* state, const UserType* ptr) {
		return Value<UserType>::push(state, *ptr);
	}
};

/**
 * Register the metatable for user type `UserType`. This function allows you to register methods
 * which are shared across all instances of this type.
 *
 * By default a garbage-collector hook and string representation function are added as meta methods.
 * Both can be overwritten.
 *
 * \tparam UserType User type struct or class
 *
 * \param state        Lua state
 * \param methods      Map of methods
 * \param meta_methods Map of meta methods
 */
template <typename UserType> static inline
void registerUserType(
	State* state,
	const MemberMap& methods = MemberMap(),
	const MemberMap& meta_methods = MemberMap()
) {
	using Wrapper = internal::UserTypeWrapper<UserType>;

	// Setup an appropriate metatable name
	luaL_newmetatable(state, Wrapper::name.c_str());

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

/**
 * Same as the other `registerUserType` but registers the constructor as well. The template
 * parameter is a signature `UserType(Args...)` where `UserType` is the user type and `Args...` its
 * constructor parameters types.
 */
template <typename Sig> static inline
void registerUserType(
	State* state,
	const char* ctor_name,
	const MemberMap& methods = MemberMap(),
	const MemberMap& meta_methods = MemberMap()
) {
	using UserType =
		typename internal::StripUserType<
			typename internal::CallableInfo<Sig>::ReturnType
		>;

	registerUserType<UserType>(state, methods, meta_methods);

	setGlobal(
		state,
		ctor_name,
		&internal::CallableInfo<Sig>::template RelayArguments<
			// Relay parameter type list to this template and return the resulting type, which is
			// internal::UserTypeWrapper<UserType>::ConstructorWrapper<Args...>.
			internal::UserTypeWrapper<UserType>::template ConstructorWrapper
		>::invoke
	);
}

LUWRA_NS_END

/**
 * Generate a user type member manifest. This is basically any type which can be constructed using a
 * string and a `lua_CFunction`. For example `std::pair<Pushable, Pushable>`.
 */
#define LUWRA_MEMBER(type, name) \
	{#name, LUWRA_WRAP_MEMBER(type, name)}

/**
 * Generate a `lua_CFunction` wrapper for a constructor.
 * \param type Type to instantiate
 * \param ...  Constructor parameter types
 * \return Wrapped function as `lua_CFunction`
 */
#define LUWRA_WRAP_CONSTRUCTOR(type, ...) \
	(&luwra::internal::UserTypeWrapper<luwra::internal::StripUserType<type>>::ConstructorWrapper<__VA_ARGS__>::invoke)

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

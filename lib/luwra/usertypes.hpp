/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_USERTYPES_H_
#define LUWRA_USERTYPES_H_

#include "common.hpp"
#include "values.hpp"
#include "stack.hpp"
#include "auxiliary.hpp"

#include <utility>
#include <string>

LUWRA_NS_BEGIN

namespace internal {
	template <typename UserType>
	using StripUserType = typename std::remove_cv<UserType>::type;

	// User type registry identifiers
	template <typename UserType>
	struct UserTypeReg {
		// Registry name for a metatable which is associated with a user type
		static
		const std::string name;
	};

	#ifndef LUWRA_REGISTRY_PREFIX
		#define LUWRA_REGISTRY_PREFIX "Luwra#"
	#endif

	template <typename UserType>
	const std::string UserTypeReg<UserType>::name =
		LUWRA_REGISTRY_PREFIX + std::to_string(uintptr_t(&name));

	template <typename UserType>
	struct UserTypeWrapper: UserTypeReg<StripUserType<UserType>> {
		using Type = StripUserType<UserType>;

		// Read the userdata instance of Type from the stack.
		static inline
		Type* check(State* state, int index) {
			return static_cast<Type*>(
				luaL_checkudata(state, index, UserTypeReg<Type>::name.c_str())
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
			push(
				state,
				UserTypeReg<Type>::name + "@" + std::to_string(uintptr_t(check(state, 1)))
			);
			return 1;
		}

		template <typename... Args>
		struct ConstructorWrapper {
			static inline
			int invoke(State* state) {
				apply(
					state,
					1,
					&Value<Type>::template push<Args...>,
					state
				);

				return 1;
			}
		};
	};
}

/// Construct a user type value on the stack.
///
/// \tparam UserType Type which you would like to construct
///
/// \param state Lua state
/// \param args  Constructor arguments
/// \returns %Reference to the constructed value
///
/// Creates a full userdata which holds an instance of the user type. A metatable that is specific
/// to `UserType` will be attached to the userdata. You can manage the metatable with
/// @ref registerUserType<UserType>.
///
/// Example:
///
/// ```
///   struct A {
///       A(const char* a, int b);
///   };
/// ```
/// ```
///   A& a = construct<A>(state, "Hello World", 11);
/// ```
template <typename UserType, typename... Args> inline
UserType& construct(State* state, Args&&... args) {
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

/// Enables reading/pushing for an arbitrary type.
template <typename UserType>
struct Value {
	/// Get a reference to a user type value on the stack.
	///
	/// \param state Lua state
	/// \param index Index of the value on the stack
	/// \returns A reference to the user type value
	static inline
	UserType& read(State* state, int index) {
		return *internal::UserTypeWrapper<UserType>::check(state, index);
	}

	/// Identical to @ref construct<UserType>.
	template <typename... Args> static inline
	void push(State* state, Args&&... args) {
		construct<UserType>(state, std::forward<Args>(args)...);
	}
};

/// Enables reading and pushing the arbitrary type `UserType`.
template <typename UserType>
struct Value<UserType*> {
	/// Get a pointer to a user type value on the stack.
	///
	/// \param state Lua state
	/// \param index Index of the value on the stack
	/// \returns A pointer to the user type value.
	static inline
	UserType* read(State* state, int index) {
		return internal::UserTypeWrapper<UserType>::check(state, index);
	}

	/// Copy a user type value onto the stack. Uses @ref construct to invoke the copy constructor.
	///
	/// \param state Lua state
	/// \param ptr   Pointer to the value
	static inline
	void push(State* state, const UserType* ptr) {
		construct<UserType>(state, *ptr);
	}
};

/// Register the metatable for a user type. This function allows you to register properties which
/// are shared across all instances of the user type.
///
/// \tparam UserType Type for which the metatable will be registered
///
/// \param state Lua state
/// \param props Properties of the user type
/// \param meta  Meta methods of the user type
///
/// By default, a garbage-collector hook and string representation function are added as meta
/// methods. Both can be overwritten.
///
/// Example:
///
/// ```
///   struct A {
///       int foo;
///
///       A(int x);
///
///       void bar();
///
///       A __add(const A& x);
///   };
/// ```
/// ```
///   // Register the meta table
///   registerUserType<A>(
///       state,
///       {
///           LUWRA_MEMBER(A, foo),
///           LUWRA_MEMBER(A, bar)
///       },
///       {
///           LUWRA_MEMBER(A, __add)
///       }
///   );
///
///   // Register the constructor in the global namespace
///   setGlobal(state, "A", LUWRA_WRAP_CONSTRUCTOR(A, int));
/// ```
///
/// in Lua
///
/// ```
///   local x = A(13)
///
///   -- Retrieve 'foo'
///   x:foo()
///
///   -- Update 'foo'
///   x:foo(37)
///
///   -- Call 'bar'
///   x:bar()
///
///   -- Use meta method '__add'
///   local y = x + A(-16)
/// ```
template <typename UserType> inline
void registerUserType(
	State*           state,
	const MemberMap& props = MemberMap(),
	const MemberMap& meta = MemberMap()
) {
	using Wrapper = internal::UserTypeWrapper<UserType>;

	// Setup an appropriate metatable name
	luaL_newmetatable(state, Wrapper::name.c_str());

	// Set fields of the metatable
	setFields(state, -1,
		"__index",    props,
		"__gc",       &Wrapper::destruct,
		"__tostring", &Wrapper::stringify
	);

	// Insert meta methods
	setFields(state, -1, meta);

	// Pop metatable off the stack
	lua_pop(state, -1);
}

/// Same as the other @ref registerUserType but registers a constructor in the global namespace.
///
/// \tparam Sig A signature in the form of `UserType(CtorArgs...)` where `UserType` is the user type
///             for which you would like to register the metatable and `CtorArgs...` the parameter
///             types of the constructor.
///
/// \param state     Lua state
/// \param ctor_name Constructor name
/// \param props     Properties
/// \param meta      Meta methods
template <typename Sig> inline
void registerUserType(
	State*           state,
	const char*      ctor_name,
	const MemberMap& props = MemberMap(),
	const MemberMap& meta = MemberMap()
) {
	using UserType = internal::StripUserType<internal::ReturnTypeOf<Sig>>;

	registerUserType<UserType>(state, props, meta);

	setGlobal(
		state,
		ctor_name,
		&internal::ArgumentsOf<Sig>::template Relay<
			// Relay parameter type list to this template and return the resulting type, which is
			// internal::UserTypeWrapper<UserType>::ConstructorWrapper<Args...>.
			internal::UserTypeWrapper<UserType>::template ConstructorWrapper
		>::invoke
	);
}

LUWRA_NS_END

/// Generate a user type member manifest. This is basically any type which can be constructed using
/// a string and a `lua_CFunction`. For example `std::pair<Pushable, Pushable>`.
///
/// \param type User type
/// \param name Member name
///
/// Example:
///
/// ```
///   struct A {
///       void foo();
///   };
///
///   // ...
///
///   // LUWRA_MEMBER(A, foo) == {"foo", LUWRA_WRAP_MEMBER(A, foo)}
///   MemberMap members {
///       LUWRA_MEMBER(A, foo)
///   };
/// ```
#define LUWRA_MEMBER(type, name) \
	{#name, LUWRA_WRAP_MEMBER(type, name)}

/// Generate a `lua_CFunction` wrapper for a constructor.
///
/// \param type Type to instantiate
/// \param ...  Constructor parameter types
///
/// Example:
///
/// ```
///   struct A {
///       A(const char* a, int b);
///   };
///
///   // ...
///
///   lua_CFunction ctor = LUWRA_WRAP_CONSTRUCTOR(A, const char*, int);
///   setGlobal(state, "A", ctor);
/// ```
///
/// in Lua
///
/// ```
///   local a = A("Hello World", 11)
/// ```
#define LUWRA_WRAP_CONSTRUCTOR(type, ...) \
	(&luwra::internal::UserTypeWrapper<type>::ConstructorWrapper<__VA_ARGS__>::invoke)

/// Define the registry name for a user type. This macro has to be used outside of any namespace.
///
/// \param type    User type
/// \param regname Registry name
#define LUWRA_DEF_REGISTRY_NAME(type, regname) \
	LUWRA_NS_BEGIN \
	namespace internal { \
		template <> struct UserTypeReg<type> { static const std::string name; }; \
		const std::string UserTypeReg<type>::name = (regname); \
	} \
	LUWRA_NS_END

#endif

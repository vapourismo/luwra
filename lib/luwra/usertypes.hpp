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
		return Apply(state, std::function<int(A...)>([state](A... args) {
			return Value<T&>::Push(state, args...);
		}));
	}

	template <typename T>
	int UserTypeDestructor(State* state) {
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
 * Instances of user types shall always be used as references, because Lua values can not be
 * referenced, hence this allows the compiler to differentiate between them.
 * The life-time of such instance is determined by Lua.
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
 * Register the metatable for the user type `T`. This function allows you to register methods
 * which are shared across all instances of this type. A garbage-collector hook is also inserted;
 * it destructs the underlying type when the garbage-collector says it is time to say good-bye.
 */
template <typename T> static inline
void RegisterUserType(
	State* state,
	std::initializer_list<std::pair<const char*, CFunction>> methods,
	std::initializer_list<std::pair<const char*, CFunction>> meta_methods = {}
) {
	// Setup an appropriate meta table name
	if (internal::UserTypeName<T>.empty())
		internal::UserTypeName<T> = "UD#" + std::to_string(internal::UserTypeCounter++);

	luaL_newmetatable(state, internal::UserTypeName<T>.c_str());

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
	lua_pushcfunction(state, &internal::UserTypeDestructor<T>);
	lua_rawset(state, -3);

	// Register string representation function
	lua_pushstring(state, "__tostring");
	lua_pushcfunction(state, &internal::UserTypeToString<T>);
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

LUWRA_NS_END

#endif

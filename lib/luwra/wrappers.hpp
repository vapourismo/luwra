/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_WRAPPERS_H_
#define LUWRA_WRAPPERS_H_

#include "common.hpp"
#include "types.hpp"
#include "stack.hpp"

LUWRA_NS_BEGIN

namespace internal {
	// Specializations of this template shall provide a static function `int invoke(State* state)`,
	// which serves as an instance of `lua_CFunction`.
	// Pointers to these functions can be passed to the Lua VM, so that their functionality is
	// available within a Lua program.
	template <typename T>
	struct GenericWrapper {
		static_assert(
			sizeof(T) == -1,
			"Template parameter to GenericWrapper is not valid"
		);
	};

	// This wraps generic functions. All parameters are read off the stack using their respective
	// `Value` specialization and subsequently passed to the function. The returned value will be
	// pushed onto the stack.
	template <typename R, typename... A>
	struct GenericWrapper<R(A...)> {
		template <R (* fun)(A...)> static inline
		int invoke(State* state) {
			return static_cast<int>(
				map<R(A...)>(state, fun)
			);
		}
	};

	// An alias for the `R(A...)` specialization. It primarily exists because functions aren't
	// passable as values, instead they are referenced using a function pointer.
	template <typename R, typename... A>
	struct GenericWrapper<R (*)(A...)>: GenericWrapper<R(A...)> {};

	// To avoid repeating the same code for the several types of method pointers, the base code is
	// unified here.
	template <typename MP, typename T, typename R, typename... A>
	struct MethodWrapperImpl {
		template <MP meth> static inline
		R hook(T* parent, A&&... args) {
			return (parent->*meth)(std::forward<A>(args)...);
		}

		template <MP meth> static inline
		int invoke(State* state) {
			return static_cast<int>(
				map<R(T*, A...)>(state, hook<meth>)
			);
		}
	};

	// Wrap methods that expect `this` to be 'const volatile'-qualified.
	template <typename T, typename R, typename... A>
	struct GenericWrapper<R (T::*)(A...) const volatile>:
		MethodWrapperImpl<R (T::*)(A...) const volatile, T, R, A...>
	{};

	// Wrap methods that expect `this` to be 'const'-qualified.
	template <typename T, typename R, typename... A>
	struct GenericWrapper<R (T::*)(A...) const>:
		MethodWrapperImpl<R (T::*)(A...) const, T, R, A...>
	{};

	// Wrap methods that expect `this` to be 'volatile'-qualified.
	template <typename T, typename R, typename... A>
	struct GenericWrapper<R (T::*)(A...) volatile>:
		MethodWrapperImpl<R (T::*)(A...) volatile, T, R, A...>
	{};

	// Wrap methods that expect `this` to be unqualified.
	template <typename T, typename R, typename... A>
	struct GenericWrapper<R (T::*)(A...)>:
		MethodWrapperImpl<R (T::*)(A...), T, R, A...>
	{};

	// Wrap a 'const'-qualified field accessor. Because the field can not be changed, this wrapper
	// does not provide a setter mechanism.
	template <typename T, typename R>
	struct GenericWrapper<const R T::*> {
		template <const R T::* accessor> static inline
		int invoke(State* state) {
			return static_cast<int>(
				push(state, read<T*>(state, 1)->*accessor)
			);
		}
	};

	// Wrap a field accessor. The wrapper provides both setter and getter mechanism.
	template <typename T, typename R>
	struct GenericWrapper<R T::*> {
		template <R T::* accessor> static inline
		int invoke(State* state) {
			if (lua_gettop(state) > 1) {
				read<T*>(state, 1)->*accessor = read<R>(state, 2);
				return 0;
			} else {
				return static_cast<int>(
					push(state, read<T*>(state, 1)->*accessor)
				);
			}
		}
	};
}

LUWRA_NS_END

/**
 * Generate a `lua_CFunction` wrapper for a field, method or function.
 * \returns Wrapped entity as `lua_CFunction`
 */
#define LUWRA_WRAP(entity) \
	(&luwra::internal::GenericWrapper<decltype(&entity)>::template invoke<&entity>)

/**
 * Generate a user type member manifest. This is basically any type which can be constructed using a
 * string and a `lua_CFunction`. For example `std::pair<Pushable, Pushable>`.
 */
#define LUWRA_MEMBER(type, name) \
	{#name, LUWRA_WRAP(__LUWRA_NS_RESOLVE(type, name))}

#endif

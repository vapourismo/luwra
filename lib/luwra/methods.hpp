/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_METHODS_H_
#define LUWRA_METHODS_H_

#include "common.hpp"
#include "stack.hpp"
#include "functions.hpp"

LUWRA_NS_BEGIN

namespace internal {
	/**
	 * Helper struct for wrapping user type methods
	 */
	template <typename T>
	struct MethodWrapper {
		static_assert(
			sizeof(T) == -1,
			"Undefined template MethodWrapper"
		);
	};

	// 'const volatile'-qualified methods
	template <typename T, typename R, typename... A>
	struct MethodWrapper<R (T::*)(A...) const volatile> {
		using MethodPointerType = R (T::*)(A...) const volatile;
		using FunctionSignature = R (const volatile T*, A...);

		template <MethodPointerType meth> static inline
		R hook(const volatile T* parent, A&&... args) {
			return (parent->*meth)(std::forward<A>(args)...);
		}

		template <MethodPointerType meth> static inline
		int invoke(State* state) {
			return static_cast<int>(
				map<FunctionSignature>(state, hook<meth>)
			);
		}
	};

	// 'const'-qualified methods
	template <typename T, typename R, typename... A>
	struct MethodWrapper<R (T::*)(A...) const> {
		using MethodPointerType = R (T::*)(A...) const;
		using FunctionSignature = R (const T*, A...);

		template <MethodPointerType meth> static inline
		R hook(const T* parent, A... args) {
			return (parent->*meth)(std::forward<A>(args)...);
		}

		template <MethodPointerType meth> static inline
		int invoke(State* state) {
			return static_cast<int>(
				map<FunctionSignature>(state, hook<meth>)
			);
		}
	};

	// 'volatile'-qualified methods
	template <typename T, typename R, typename... A>
	struct MethodWrapper<R (T::*)(A...) volatile> {
		using MethodPointerType = R (T::*)(A...) volatile;
		using FunctionSignature = R (volatile T*, A...);

		template <MethodPointerType meth> static inline
		R hook(volatile T* parent, A... args) {
			return (parent->*meth)(std::forward<A>(args)...);
		}

		template <MethodPointerType meth> static inline
		int invoke(State* state) {
			return static_cast<int>(
				map<FunctionSignature>(state, hook<meth>)
			);
		}
	};

	// unqualified methods
	template <typename T, typename R, typename... A>
	struct MethodWrapper<R (T::*)(A...)> {
		using MethodPointerType = R (T::*)(A...);
		using FunctionSignature = R (T*, A...);

		template <MethodPointerType meth> static inline
		R hook(T* parent, A... args) {
			return (parent->*meth)(std::forward<A>(args)...);
		}

		template <MethodPointerType meth> static inline
		int invoke(State* state) {
			return static_cast<int>(
				map<FunctionSignature>(state, hook<meth>)
			);
		}
	};
}

LUWRA_NS_END

/**
 * Generate a `lua_CFunction` wrapper for a method.
 * \param meth Fully qualified method name (Do not supply a pointer)
 * \return Wrapped function as `lua_CFunction`
 */
#define LUWRA_WRAP_METHOD(meth) \
	(&luwra::internal::MethodWrapper<decltype(&meth)>::template invoke<&meth>)

#endif

/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_STACK_H_
#define LUWRA_STACK_H_

#include "common.hpp"
#include "types.hpp"
#include "internal.hpp"

#include <cassert>
#include <utility>
#include <functional>
#include <string>

LUWRA_NS_BEGIN

namespace internal {
	template <typename Sig>
	struct Layout {
		static_assert(
			sizeof(Sig) == -1,
			"Parameter to Layout is not a valid signature"
		);
	};

	template <>
	struct Layout<void ()> {
		using ReturnType = void;

		template <typename Callable, typename... Args> static inline
		void direct(State*, int, Callable&& hook, Args&&... args) {
			hook(
				std::forward<Args>(args)...
			);
		}
	};

	template <typename Ret>
	struct Layout<Ret ()> {
		using ReturnType = Ret;

		template <typename Callable, typename... Args> static inline
		Ret direct(State*, int, Callable&& hook, Args&&... args) {
			return hook(
				std::forward<Args>(args)...
			);
		}
	};

	template <typename Arg>
	struct Layout<void (Arg)> {
		using ReturnType = void;

		template <typename Callable, typename... Args> static inline
		void direct(State* state, int n, Callable&& hook, Args&&... args) {
			hook(
				std::forward<Args>(args)...,
				Value<Arg>::read(state, n)
			);
		}
	};

	template <typename Ret, typename Arg>
	struct Layout<Ret (Arg)> {
		using ReturnType = Ret;

		template <typename Callable, typename... Args> static inline
		Ret direct(State* state, int n, Callable&& hook, Args&&... args) {
			return hook(
				std::forward<Args>(args)...,
				Value<Arg>::read(state, n)
			);
		}
	};

	template <typename Ret, typename Head, typename... Tail>
	struct Layout<Ret (Head, Tail...)> {
		using ReturnType = Ret;

		template <typename Callable, typename... Args> static inline
		Ret direct(State* state, int n, Callable&& hook, Args&&... args) {
			return Layout<Ret (Tail...)>::direct(
				state,
				n + 1,
				std::forward<Callable>(hook),
				std::forward<Args>(args)...,
				Value<Head>::read(state, n)
			);
		}
	};
}

/**
 * Retrieve values from the stack and invoke a `Callable` with them.
 *
 * \tparam Sig       Signature in the form of `R(A...)` where `A` is a sequence of types, which
 *                   shall be retrieved from the stack, and `R` the return type of `func`
 * \tparam Callable  An instance of `Callable` which accepts parameters `X..., A...` and returns `R`
 *                   (this parameter should be inferable and can be omitted)
 * \tparam ExtraArgs Extra argument types (can be omitted)
 *
 * \param state Lua state instance
 * \param pos   Index of the first value
 * \param func  Callable value
 * \param args  Extra arguments which shall be be passed to `func` before the stack values
 *
 * \returns Result of calling `func`
 */
template <typename Sig, typename Callable, typename... ExtraArgs> static inline
typename internal::Layout<Sig>::ReturnType direct(
	State*         state,
	int            pos,
	Callable&&     func,
	ExtraArgs&&... args
) {
	return internal::Layout<Sig>::direct(
		state,
		pos,
		std::forward<Callable>(func),
		std::forward<ExtraArgs>(args)...
	);
}

namespace internal {
	template <typename Ret>
	struct ApplyLayout {
		template <typename... Args>
		using Type = Layout<Ret (Args...)>;
	};
}

/**
 * A version of [direct](@ref direct) which tries to infer the stack layout from the given
 * `Callable`. It allows you to omit the template parameters since the compiler is able to infer the
 * parameter and return types.
 */
template <typename Callable, typename... ExtraArgs> static inline
typename internal::CallableInfo<Callable>::ReturnType apply(
	State*         state,
	int            pos,
	Callable&&     func,
	ExtraArgs&&... args
) {
	using ExtraArgList = internal::TypeList<ExtraArgs...>;
	using CallableArgList = typename internal::CallableInfo<Callable>::ArgumentTypeList;

	static_assert(
		internal::IsPrefixOf<
			std::is_convertible,
			ExtraArgList,
			CallableArgList
		>::value,
		"Given extra arguments cannot be passed to the provided Callable"
	);

	using StackTypeList = typename CallableArgList::template Drop<sizeof...(ExtraArgs)>;
	using ReturnType = typename internal::CallableInfo<Callable>::ReturnType;

	using Sig =
		typename StackTypeList::template Relay<
			internal::With<ReturnType>::template ConstructSignature
		>;

	return internal::Layout<Sig>::direct(
		state,
		pos,
		std::forward<Callable>(func),
		std::forward<ExtraArgs>(args)...
	);
}

namespace internal {
	template <typename Type>
	struct SpecialValuePusher;

	template <typename Seq>
	struct TuplePusher {
		static_assert(
			sizeof(Seq) == -1,
			"Template parameter to TuplePusher is not an IndexSequence"
		);
	};

	template <size_t Index>
	struct TuplePusher<IndexSequence<Index>> {
		template <typename... Contents> static inline
		size_t push(State* state, const std::tuple<Contents...>& package) {
			return SpecialValuePusher<
				typename std::tuple_element<Index, std::tuple<Contents...>>::type
			>::push(state, std::get<Index>(package));
		}
	};

	template <size_t Index, size_t... IndexPack>
	struct TuplePusher<IndexSequence<Index, IndexPack...>> {
		template <typename... Contents> static inline
		size_t push(State* state, const std::tuple<Contents...>& package) {
			return
				TuplePusher<IndexSequence<Index>>::push(state, package) +
				TuplePusher<IndexSequence<IndexPack...>>::push(state, package);
		}
	};

	template <typename Type>
	struct SpecialValuePusher {
		template <typename... Args> static inline
		size_t push(State* state, Args&&... args) {
			Value<Type>::push(state, std::forward<Args>(args)...);
			return 1;
		}
	};

	template <typename... Contents>
	struct SpecialValuePusher<std::tuple<Contents...>> {
		static inline
		size_t push(State* state, const std::tuple<Contents...>& value) {
			using Seq = internal::MakeIndexSequence<sizeof...(Contents)>;
			return TuplePusher<Seq>::push(state, value);
		};
	};

	template <typename Sig>
	struct LayoutMapper {
		static_assert(
			sizeof(Sig) == -1,
			"Parameter to LayoutMapper is not a valid signature"
		);
	};

	template <typename... Args>
	struct LayoutMapper<void (Args...)> {
		template <typename Callable, typename... ExtraArgs> static inline
		size_t map(State* state, int n, Callable&& hook, ExtraArgs&&... args) {
			direct<void (Args...)>(
				state,
				n,
				std::forward<Callable>(hook),
				std::forward<ExtraArgs>(args)...
			);
			return 0;
		}
	};

	template <typename Ret, typename... Args>
	struct LayoutMapper<Ret (Args...)> {
		template <typename Callable, typename... ExtraArgs> static inline
		size_t map(State* state, int n, Callable&& hook, ExtraArgs&&... args) {
			return SpecialValuePusher<Ret>::push(
				state,
				direct<Ret (Args...)>(
					state,
					n,
					std::forward<Callable>(hook),
					std::forward<ExtraArgs>(args)...
				)
			);
		}
	};
}

/**
 * Similar to [direct](@ref direct) but pushes the result of the given `Callable` onto the stack.
 * \returns Number of values pushed
 */
template <typename Sig, typename Callable, typename... Args> static inline
size_t map(State* state, int pos, Callable&& hook, Args&&... args) {
	return internal::LayoutMapper<Sig>::map(
		state,
		pos,
		std::forward<Callable>(hook),
		std::forward<Args>(args)...
	);
}

LUWRA_NS_END

#endif

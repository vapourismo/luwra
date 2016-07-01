/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_STACK_H_
#define LUWRA_STACK_H_

#include "common.hpp"
#include "types.hpp"
#include "internal/typelist.hpp"
#include "internal/types.hpp"

#include <cassert>
#include <utility>
#include <functional>
#include <string>

LUWRA_NS_BEGIN

namespace internal {
	// Catch usage error.
	template <typename Seq, typename...>
	struct _StackWalker {
		static_assert(
			sizeof(Seq) == -1,
			"Invalid template parameters to _StackWalker"
		);
	};

	// Collect values from the stack and call a Callable with them.
	template <size_t... Indices, typename... Types>
	struct _StackWalker<IndexSequence<Indices...>, Types...> {
		template <typename Callable, typename... Args> static inline
		ReturnTypeOf<Callable> walk(State* state, int pos, Callable&& func, Args&&... args) {
			return func(
				std::forward<Args>(args)...,
				luwra::read<Types>(state, pos + Indices)...
			);
		}
	};

	template <typename... Types>
	using StackWalker = _StackWalker<MakeIndexSequence<sizeof...(Types)>, Types...>;
}

/**
 *
 */
template <typename Callable, typename... ExtraArgs> static inline
internal::ReturnTypeOf<Callable> apply(
	State*         state,
	int            pos,
	Callable&&     func,
	ExtraArgs&&... args
) {
	using ExtraArgList = internal::TypeList<ExtraArgs...>;
	using CallableArgList = internal::ArgumentsOf<Callable>;

	static_assert(
		ExtraArgList::template PrefixOf<
			std::is_convertible,
			CallableArgList
		>::value,
		"Given extra arguments cannot be passed to the provided Callable"
	);

	using StackArgList = typename CallableArgList::template Drop<sizeof...(ExtraArgs)>;
	using Walker = typename StackArgList::template Relay<internal::StackWalker>;

	return Walker::walk(
		state,
		pos,
		std::forward<Callable>(func),
		std::forward<ExtraArgs>(args)...
	);
}

namespace internal {
	template <typename>
	struct StackMapper {
		template <typename Callable, typename... ExtraArgs> static inline
		size_t map(State* state, int pos, Callable&& func, ExtraArgs&&... args) {
			return pushReturn(
				state,
				apply(
					state,
					pos,
					std::forward<Callable>(func),
					std::forward<ExtraArgs>(args)...
				)
			);
		}
	};

	template <>
	struct StackMapper<void> {
		template <typename Callable, typename... ExtraArgs> static inline
		size_t map(State* state, int pos, Callable&& func, ExtraArgs&&... args) {
			apply(
				state,
				pos,
				std::forward<Callable>(func),
				std::forward<ExtraArgs>(args)...
			);

			return 0;
		}
	};
}

/**
 *
 */
template <typename Callable, typename... ExtraArgs> static inline
size_t map(State* state, int pos, Callable&& func, ExtraArgs&&... args) {
	return internal::StackMapper<internal::ReturnTypeOf<Callable>>::map(
		state,
		pos,
		std::forward<Callable>(func),
		std::forward<ExtraArgs>(args)...
	);
}

LUWRA_NS_END

#endif

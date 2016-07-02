/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_STACK_H_
#define LUWRA_STACK_H_

#include "common.hpp"
#include "values.hpp"
#include "internal/typelist.hpp"
#include "internal/types.hpp"

#include <utility>

LUWRA_NS_BEGIN

/// Push a value onto the stack.
///
/// \param state Lua state
/// \param value To be pushed
template <typename Type> inline
void push(State* state, Type&& value) {
	Value<Type>::push(state, std::forward<Type>(value));
}

/// Push two or more values onto the stack.
///
/// \param state  Lua state
/// \param first  First value
/// \param second Second value
/// \param rest   Remaining values
template <typename First, typename Second, typename... Rest> inline
void push(State* state, First&& first, Second&& second, Rest&&... rest) {
	push(state, std::forward<First>(first));
	push(state, std::forward<Second>(second), std::forward<Rest>(rest)...);
}

/// Read a value off the stack.
///
/// \tparam Type  Type of targeted value
/// \param  state Lua state
/// \param  index Position of the value on the stack
template <typename Type> inline
auto read(State* state, int index) -> decltype(Value<Type>::read(state, index)) {
	return Value<Type>::read(state, index);
}

/// Push a return value onto the stack.
///
/// \param state Lua state
/// \param value Return value
/// \returns Number of Lua values that have been pushed onto the stack
template <typename Type> inline
size_t pushReturn(State* state, Type&& value) {
	return ReturnValue<Type>::push(state, std::forward<Type>(value));
}

/// Push multiple return values onto the stack.
///
/// \param state   Lua state
/// \param first   First return value
/// \param second  Second return value
/// \param rest    More return values
/// \returns Number of Lua values that have been pushed onto the stack
template <typename First, typename Second, typename... Rest> inline
size_t pushReturn(State* state, First&& first, Second&& second, Rest&&... rest) {
	return
		pushReturn(state, std::forward<First>(first)) +
		pushReturn(state, std::forward<Second>(second), std::forward<Rest>(rest)...);
}

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

	template <typename... Types>
	using ReadResults = TypeList<ReturnTypeOf<decltype(read<Types>)>...>;
}

/// Retrieve values from the stack in order to invoke a `Callable` with them.
///
/// \param state Lua state
/// \param pos   Index of the first stack value
/// \param func  A `Callable`
/// \param args  Extra arguments passed to `func` before the stack values
/// \returns Result of calling `func`
///
/// Invoke `func` with `args...` followed by the values retrieved from the stack.
///
/// Example 1:
///
/// ```
///   double sum(double a, double b) {
///       return a + b;
///   }
///
///   // ...
///
///   push(state, 37.13);
///   push(state, 13.37);
///
///   // All parameters are extracted from the stack.
///   double result = apply(state, 1, sum);
/// ```
///
/// Example 2:
///
/// ```
///   push(state, 37.13);
///   push(state, 13.37);
///
///   // Only parameters 'b' and 'c' are extracted from the stack.
///   double result = apply(state, 1, [](double a, double b, double c) {
///       return a + b + c;
///   }, -0.5);
/// ```
template <typename Callable, typename... ExtraArgs> inline
internal::ReturnTypeOf<Callable> apply(
	State*         state,
	int            pos,
	Callable&&     func,
	ExtraArgs&&... args
) {
	using ExtraArgList = internal::TypeList<ExtraArgs...>;
	using CallableArgList = internal::ArgumentsOf<Callable>;

	// Make sure the extra arguments can be passed to 'func'.
	static_assert(
		ExtraArgList::template PrefixOf<
			std::is_convertible,
			CallableArgList
		>::value,
		"Given extra arguments cannot be passed to the provided Callable"
	);

	using StackArgList = typename CallableArgList::template Drop<sizeof...(ExtraArgs)>;
	using ReadArgList = typename StackArgList::template Relay<internal::ReadResults>;

	// Make sure that the results of 'read' operations match the required stack parameter types.
	static_assert(
		ReadArgList::template Match<
			std::is_convertible,
			StackArgList
		>::value,
		"Given Callable expects values to be extracted in ways that are not possible"
	);

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

/// Retrieve values from the stack in order to invoke a `Callable` with them, then push the result
/// onto the stack.
///
/// \param state Lua state
/// \param pos   Index of the first stack value
/// \param func  A `Callable`
/// \param args  Extra arguments passed to `func` before the stack values
/// \returns Number of return values push onto stack
///
/// Works similar to [apply](@ref apply). This function pushes the result of `func` onto the stack.
template <typename Callable, typename... ExtraArgs> inline
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

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
#include <numeric>

LUWRA_NS_BEGIN

/// Push zero or more values onto the stack.
///
/// \param state  Lua state
/// \param values Values to push
template <typename... Types> inline
void push(State* state, Types&&... values) {
	using Ew = int[];
	(void) Ew {(Value<Types>::push(state, std::forward<Types>(values)), 0)...};
}

/// Read a value off the stack.
///
/// \tparam Type  Type of targeted value
/// \param  state Lua state
/// \param  index Position of the value on the stack
template <typename Type = internal::InferValueType> inline
auto read(State* state, int index) -> decltype(Value<Type>::read(state, index)) {
	return Value<Type>::read(state, index);
}

/// Push zero or more return values onto the stack.
///
/// \param state  Lua state
/// \param values Values to push
/// \returns Number of Lua values that have been pushed onto the stack
template <typename... Types> inline
size_t pushReturn(State* state, Types&&... values) {
	size_t results[sizeof...(Types)] {
		ReturnValue<Types>::push(state, std::forward<Types>(values))...
	};

	size_t sum = 0;
	for (size_t i = 0; i < sizeof...(Types); i++)
		sum += results[i];

	return sum;
}

namespace internal {
	template <typename... Types>
	struct _StackWalker {
		template <size_t... Indices>
		struct Walker {
			template <typename Callable, typename... Args> static inline
			ReturnTypeOf<Callable> walk(State* state, int pos, Callable&& func, Args&&... args) {
				return func(
					std::forward<Args>(args)...,
					luwra::read<Types>(state, pos + Indices)...
				);
			}
		};
	};

	template <typename... Types>
	using StackWalker =
		typename MakeIndexSequence<sizeof...(Types)>::template Relay<
			_StackWalker<Types...>::template Walker
		>;

	// Why you may ask? Because VS 2015.
	template <typename... Types>
	struct _ReadResults {
		using Type = TypeList<ReturnTypeOf<decltype(read<Types>)>...>;
	};

	template <>
	struct _ReadResults<> {
		using Type = TypeList<>;
	};

	template <typename... Types>
	using ReadResults = typename _ReadResults<Types...>::Type;
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
/// ```
/// ```
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
/// \returns Number of return values pushed onto the stack
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

/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_INTERNAL_TYPELIST_HPP
#define LUWRA_INTERNAL_TYPELIST_HPP

#include "common.hpp"
#include "indexsequence.hpp"

LUWRA_INTERNAL_NS_BEGIN

template <typename... Types>
struct TypeList;

// Not sure what to do.
template <typename Seq, typename... Types>
struct _DropFromList {
	using Result = TypeList<Types...>;
};

// Drop 0 types from the list.
template <typename... Types>
struct _DropFromList<IndexSequence<>, Types...> {
	using Result = TypeList<Types...>;
};

// Drop 1 or more types from the list.
template <size_t Index, size_t... IndexTail, typename Head, typename... Tail>
struct _DropFromList<IndexSequence<Index, IndexTail...>, Head, Tail...>:
	_DropFromList<IndexSequence<IndexTail...>, Tail...> {};

// Usage error, default to false.
template <
	template <typename, typename> class,
	typename,
	typename
>
struct _PrefixOf:
	BoolConstant<false> {};

// Empty prefix is a valid prefix for anything.
template <
	template <typename, typename> class Compare,
	typename... Types
>
struct _PrefixOf<Compare, TypeList<>, TypeList<Types...>>:
	BoolConstant<true> {};

// Iterate through prefix TypeList and check each.
template <
	template <typename, typename> class Compare,
	typename Prefix,
	typename... PrefixTail,
	typename Base,
	typename... BaseTail
>
struct _PrefixOf<Compare, TypeList<Prefix, PrefixTail...>, TypeList<Base, BaseTail...>>:
	BoolConstant<
		Compare<Prefix, Base>::value &&
		_PrefixOf<Compare, TypeList<PrefixTail...>, TypeList<BaseTail...>>::value
	> {};

// Match two types
template <
	template <typename, typename> class Compare,
	typename Left,
	typename Right
>
struct _Match:
	Compare<Left, Right> {};

// Match two empty TypeLists
template <template <typename, typename> class Compare>
struct _Match<Compare, TypeList<>, TypeList<>>:
	BoolConstant<true> {};

// Match non-empty TypeLists
template <
	template <typename, typename> class Compare,
	typename Left,
	typename... LeftTail,
	typename Right,
	typename... RightTail
>
struct _Match<Compare, TypeList<Left, LeftTail...>, TypeList<Right, RightTail...>>:
	BoolConstant<
		sizeof...(LeftTail) == sizeof...(RightTail) &&
		Compare<Left, Right>::value &&
		_Match<Compare, TypeList<LeftTail...>, TypeList<RightTail...>>::value
	> {};

// Exchanging more than one template parameter pack can be cumbersome. This template helps to
// simplify this by containing the type list within its own template parameters. One can extract the
// types by matching against this template's parameters.
template <typename... Types>
struct TypeList {
	// Number of elements in the parameter pack.
	constexpr static
	size_t Length = sizeof...(Types);

	// Create a new TypeList which contains the current list of types followed by the ones given to
	// this alias.
	template <typename... OtherTypes>
	using Add = TypeList<Types..., OtherTypes...>;

	// Pass the parameter pack to the provided template.
	template <template <typename...> class Receiver>
	using Relay = Receiver<Types...>;

	// Drop N types from the front list.
	template <size_t N>
	using Drop = typename _DropFromList<MakeIndexSequence<N>, Types...>::Result;

	// Create a new TypeList which unifies the types contained in this and another TypeList.
	template <typename Other>
	using Append = typename Other::template Relay<TypeList<Types...>::template Add>;

	// Check if this TypeList is a prefix to another. Use the given template to compare types within
	// the TypeLists.
	template <template <typename, typename> class Compare, typename Other>
	using PrefixOf = _PrefixOf<Compare, TypeList<Types...>, Other>;

	// Match with another TypeList.
	template <template <typename, typename> class Compare, typename Other>
	using Match = _Match<Compare, TypeList<Types...>, Other>;
};

LUWRA_INTERNAL_NS_END

#endif

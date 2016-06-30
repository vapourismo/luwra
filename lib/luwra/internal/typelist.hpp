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

// Exchanging more than one template parameter pack can be cumbersome. This template helps to
// simplify this by containing the type list within its own template parameters. One can extract the
// types by matching against this template's parameters.
template <typename... Types>
struct TypeList {
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
};

LUWRA_INTERNAL_NS_END

#endif

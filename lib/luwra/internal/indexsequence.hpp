/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_INTERNAL_INDEXSEQUENCE_H_
#define LUWRA_INTERNAL_INDEXSEQUENCE_H_

#include "common.hpp"

LUWRA_INTERNAL_NS_BEGIN

// Capture a list of indices.
template <size_t... Seq>
struct IndexSequence {
	template <size_t Offset>
	using OffsetBy = IndexSequence<(Seq + Offset)...>;

	template <template <size_t...> class Receiver>
	using Relay = Receiver<Seq...>;
};

// Generate a sequence of indices. Count refers to the number of indices which shall prefix the
// given Indices.
template <size_t Count, size_t... Indices>
struct _MakeIndexSequence:
	_MakeIndexSequence<Count - 1, Count - 1, Indices...> {};

// Done creating the sequence.
template <size_t... Indices>
struct _MakeIndexSequence<0, Indices...> {
	using Result = IndexSequence<Indices...>;
};

// Create an IndexSequence where Count specified the number of indices.
template <size_t Count>
using MakeIndexSequence = typename _MakeIndexSequence<Count>::Result;

LUWRA_INTERNAL_NS_END

#endif

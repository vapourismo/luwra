/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_COMPAT_H_
#define LUWRA_COMPAT_H_

#include "common.hpp"

LUWRA_NS_BEGIN

namespace internal {
	template <size_t... Is>
	struct IndexSequence {};

	template <size_t I, size_t... Is>
	struct MakeIndexSequenceImpl {
		using type = typename MakeIndexSequenceImpl<I - 1, I - 1, Is...>::type;
	};

	template <size_t... Is>
	struct MakeIndexSequenceImpl<0, Is...> {
		using type = IndexSequence<Is...>;
	};

	template <size_t I>
	using MakeIndexSequence = typename MakeIndexSequenceImpl<I>::type;
}

LUWRA_NS_END

#ifdef _MSC_VER // Because VS C++
	#define __LUWRA_NS_RESOLVE(a, b) a::##b
#else
	#define __LUWRA_NS_RESOLVE(a, b) a::b
#endif

#endif

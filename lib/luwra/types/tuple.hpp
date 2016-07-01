/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_TYPES_TUPLE_H_
#define LUWRA_TYPES_TUPLE_H_

#include "../common.hpp"
#include "../types.hpp"
#include "../stack.hpp"
#include "../internal/indexsequence.hpp"

#include <tuple>

LUWRA_NS_BEGIN

namespace internal {
	template <typename Seq, typename...>
	struct _TuplePusher {
		static_assert(
			sizeof(Seq) == -1,
			"Invalid template parameters to _TuplePusher"
		);
	};

	template <size_t... Indices, typename... Contents>
	struct _TuplePusher<IndexSequence<Indices...>, Contents...> {
		static inline
		size_t push(State* state, const std::tuple<Contents...>& value) {
			return pushReturn(state, std::get<Indices>(value)...);
		}
	};

	template <typename... Contents>
	using TuplePusher = _TuplePusher<MakeIndexSequence<sizeof...(Contents)>, Contents...>;
}

/// Enables `std::tuple` as return type
template <typename... Contents>
struct ReturnValue<std::tuple<Contents...>>:
	internal::TuplePusher<Contents...> {};

LUWRA_NS_END

#endif

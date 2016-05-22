/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_INTERNAL_H_
#define LUWRA_INTERNAL_H_

#include "common.hpp"

LUWRA_NS_BEGIN

namespace internal {
	// Information about function objects
	template <typename T>
	struct CallableInfo: CallableInfo<decltype(&T::operator ())> {};

	// Information about function signature
	template <typename R, typename... A>
	struct CallableInfo<R(A...)> {
		// A call to an instance would evaluate to this type
		using ReturnType = R;

		// Pass the template parameter pack to another template
		template <template <typename...> class T>
		using RelayArguments = T<A...>;

		// Pass the signature of this callable to another template
		template <template <typename> class T>
		using RelaySignature = T<R(A...)>;
	};

	// Information about function pointers
	template <typename R, typename... A>
	struct CallableInfo<R (*)(A...)>: CallableInfo<R(A...)> {};

	// Information about method pointers
	template <typename T, typename R, typename... A>
	struct CallableInfo<R (T::*)(A...)>: CallableInfo<R(A...)> {};

	template <typename T, typename R, typename... A>
	struct CallableInfo<R (T::*)(A...) const>: CallableInfo<R(A...)> {};

	template <typename T, typename R, typename... A>
	struct CallableInfo<R (T::*)(A...) volatile>: CallableInfo<R(A...)> {};

	template <typename T, typename R, typename... A>
	struct CallableInfo<R (T::*)(A...) const volatile>: CallableInfo<R(A...)> {};

	// Useful aliases
	template <typename T>
	struct CallableInfo<const T>: CallableInfo<T> {};

	template <typename T>
	struct CallableInfo<volatile T>: CallableInfo<T> {};

	template <typename T>
	struct CallableInfo<T&>: CallableInfo<T> {};

	template <typename T>
	struct CallableInfo<T&&>: CallableInfo<T> {};
}

LUWRA_NS_END

#endif

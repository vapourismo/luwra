/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_INTERNAL_TYPES_H_
#define LUWRA_INTERNAL_TYPES_H_

#include "common.hpp"

LUWRA_INTERNAL_NS_BEGIN

// Do something with the provided type.
template <typename Type>
struct With {
	// Construct a signature.
	template <typename... Args>
	using ConstructSignature = Type (Args...);
};

LUWRA_INTERNAL_NS_END

#endif

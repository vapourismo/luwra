/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_INTERNAL_COMMON_H_
#define LUWRA_INTERNAL_COMMON_H_

#include "../common.hpp"

#include <type_traits>

#define LUWRA_INTERNAL_NS_BEGIN LUWRA_NS_BEGIN namespace internal {
#define LUWRA_INTERNAL_NS_END   LUWRA_NS_END   }

LUWRA_INTERNAL_NS_BEGIN

// Constructs a type which contains the static constant-expression boolean field 'value'.
template <bool value>
using BoolConstant = std::integral_constant<bool, value>;

LUWRA_INTERNAL_NS_END

#endif

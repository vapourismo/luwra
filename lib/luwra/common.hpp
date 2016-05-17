/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_COMMON_H_
#define LUWRA_COMMON_H_

extern "C" {
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

// Check for proper Lua version
#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 501 || LUA_VERSION_NUM >= 600
	#error Luwra has not been tested against your installed version of Lua
#endif

// Namespaces
#define LUWRA_NS_BEGIN namespace luwra {
#define LUWRA_NS_END }

// Version MAJOR.MINOR.PATCH
#define LUWRA_VERSION_MAJOR 0
#define LUWRA_VERSION_MINOR 3
#define LUWRA_VERSION_PATCH 0

#endif

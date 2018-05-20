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
#define LUWRA_VERSION_MINOR 5
#define LUWRA_VERSION_PATCH 1

// LUA_OK does not exist in Lua 5.1 and earlier
#ifndef LUA_OK
	#define LUA_OK 0
#endif

// Because VS C++
#ifdef _MSC_VER
	#define __LUWRA_NS_RESOLVE(a, b) a::##b
#else
	#define __LUWRA_NS_RESOLVE(a, b) a::b
#endif

LUWRA_NS_BEGIN

// Lua aliases
using Integer = lua_Integer;
using Number = lua_Number;
using State = lua_State;
using CFunction = lua_CFunction;

LUWRA_NS_END

#endif

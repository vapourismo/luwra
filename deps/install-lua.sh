#!/bin/sh

install_lua() {
	lua_identifier="lua-$LUA_VERSION"

	[[ -e "$lua_identifier.tar.gz" ]] || travis_retry wget -v "http://www.lua.org/ftp/$lua_identifier.tar.gz"
	tar xvfz "$lua_identifier.tar.gz"

	pushd "$lua_identifier/src"

	export LUA_LIBNAME="lua-custom"
	export LUA_SRCDIR="$(pwd)"

	make LUA_A="lib$LUA_LIBNAME.a" linux

	popd
}

install_luajit() {
	luajit_identifier="LuaJIT-$LUAJIT_VERSION"

	[[ -e "$luajit_identifier.tar.gz" ]] || travis_retry wget -v "http://luajit.org/download/$luajit_identifier.tar.gz"
	tar xvfz "$luajit_identifier.tar.gz"

	pushd "$luajit_identifier/src"

	export LUA_LIBNAME="luajit-custom"
	export LUA_SRCDIR="$(pwd)"

	make LUAJIT_A="lib$LUA_LIBNAME.a" "lib$LUA_LIBNAME.a"

	popd
}

pushd deps

[[ -n $LUA_VERSION ]] && install_lua
[[ -n $LUAJIT_VERSION ]] && install_luajit

popd
return 0

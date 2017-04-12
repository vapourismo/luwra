/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_STATEWRAPPER_H_
#define LUWRA_STATEWRAPPER_H_

#include "common.hpp"
#include "auxiliary.hpp"
#include "stack.hpp"
#include "usertypes.hpp"
#include "types/table.hpp"

#include <utility>
#include <memory>

LUWRA_NS_BEGIN

namespace internal {
	struct StateBundle {
		std::shared_ptr<State> state;

		inline
		StateBundle():
			state(luaL_newstate(), lua_close)
		{}

		inline
		StateBundle(State* other):
			state(other, [](State*) {})
		{}
	};

#if LUA_VERSION_NUM > 501
	inline
	Reference getGlobalsTable(State* state) {
		lua_rawgeti(state, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
		return {state};
	}
#endif

}

/// Wrapper for a Lua state
struct StateWrapper: internal::StateBundle, Table {
	/// Create a new Lua state.
	inline
	StateWrapper():
		internal::StateBundle(),

#if LUA_VERSION_NUM <= 501
		Table(state.get(), LUA_GLOBALSINDEX)
#else
		Table(internal::getGlobalsTable(state.get()))
#endif

	{}

	/// Operate on a foreign state instance.
	inline
	StateWrapper(State* other):
		internal::StateBundle(other),

#if LUA_VERSION_NUM <= 501
		Table(state.get(), LUA_GLOBALSINDEX)
#else
		Table(internal::getGlobalsTable(state.get()))
#endif

	{}

	/// Convert to `lua_State`.
	inline
	operator State*() const {
		return state.get();
	}

	/// Load all built-in libraries.
	inline
	void loadStandardLibrary() const {
		luaL_openlibs(state.get());
	}

	/// See [luwra::registerUserType](@ref luwra::registerUserType).
	template <typename Sig> inline
	void registerUserType(
		const char* ctor_name,
		const MemberMap& methods = MemberMap(),
		const MemberMap& meta_methods = MemberMap()
	) const {
		luwra::registerUserType<Sig>(state.get(), ctor_name, methods, meta_methods);
	}

	/// See [luwra::registerUserType](@ref luwra::registerUserType).
	template <typename UserType> inline
	void registerUserType(
		const MemberMap& methods = MemberMap(),
		const MemberMap& meta_methods = MemberMap()
	) const {
		luwra::registerUserType<UserType>(state.get(), methods, meta_methods);
	}

	/// See [luwra::push](@ref luwra::push).
	template <typename Type> inline
	void push(Type&& value) const {
		luwra::push(state.get(), std::forward<Type>(value));
	}

	/// See [luwra::push](@ref luwra::push).
	template <typename First, typename Second, typename... Rest> inline
	void push(First&& first, Second&& second, Rest&&... rest) const {
		luwra::push(
			state.get(),
			std::forward<First>(first),
			std::forward<Second>(second),
			std::forward<Rest>(rest)...
		);
	}

	/// See [luwra::push](@ref luwra::push).
	template <typename Type> inline
	const StateWrapper& operator <<(Type&& value) const {
		push(std::forward<Type>(value));
		return *this;
	}

	/// See [luwra::read](@ref luwra::read).
	template <typename Type = internal::InferValueType> inline
	Type read(int index) const {
		return luwra::read<Type>(state.get(), index);
	}

	/// See [luwra::apply](@ref luwra::apply).
	template <typename Callable, typename... ExtraArgs> inline
	internal::ReturnTypeOf<Callable> apply(
		int            pos,
		Callable&&     func,
		ExtraArgs&&... args
	) const {
		return luwra::apply(
			state.get(),
			pos,
			std::forward<Callable>(func),
			std::forward<ExtraArgs>(args)...
		);
	}

	/// See [luwra::map](@ref luwra::map).
	template <typename Callable, typename... ExtraArgs> inline
	size_t map(int pos, Callable&& func, ExtraArgs&&... args) const {
		return luwra::map(
			state.get(),
			pos,
			std::forward<Callable>(func),
			std::forward<ExtraArgs>(args)...
		);
	}

	/// See [luwra::equal](@ref luwra::equal).
	bool equal(int index1, int index2) const {
		return luwra::equal(state.get(), index1, index2);
	}

	/// See [luwra::setMetatable](@ref luwra::setMetatable).
	void setMetatable(const char* name) const {
		luwra::setMetatable(state.get(), name);
	}

	/// Execute a piece of code.
	inline
	int runString(const char* code) const {
		return luaL_dostring(state.get(), code);
	}

	/// Execute a file.
	inline
	int runFile(const char* filepath) const {
		return luaL_dofile(state.get(), filepath);
	}
};

LUWRA_NS_END

#endif

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

LUWRA_NS_BEGIN

namespace internal {
	/// Retrieve the table responsible for the global namespace.
	inline
	Table getGlobalsTable(State* state) {
		#if LUA_VERSION_NUM <= 501
			return {{state, internal::referenceValue(state, LUA_GLOBALSINDEX), false}};
		#else
			return {{state, LUA_RIDX_GLOBALS, false}};
		#endif
	}
}

/// Wrapper for a Lua state
struct StateWrapper: Table {
	State* state;
	bool close_state;

	/// Operate on a foreign state instance.
	inline
	StateWrapper(State* state):
		Table(internal::getGlobalsTable(state)),
		state(state),
		close_state(false)
	{}

	/// Create a new Lua state.
	inline
	StateWrapper():
		Table(internal::getGlobalsTable(luaL_newstate())),
		state(ref.impl->state),
		close_state(true)
	{}

	inline
	~StateWrapper() {
		if (close_state)
			lua_close(state);
	}

	/// Convert to `lua_State`.
	inline
	operator State*() const {
		return state;
	}

	/// Load all built-in libraries.
	inline
	void loadStandardLibrary() const {
		luaL_openlibs(state);
	}

	/// See [luwra::registerUserType](@ref luwra::registerUserType).
	template <typename Sig> inline
	void registerUserType(
		const char* ctor_name,
		const MemberMap& methods = MemberMap(),
		const MemberMap& meta_methods = MemberMap()
	) const {
		luwra::registerUserType<Sig>(state, ctor_name, methods, meta_methods);
	}

	/// See [luwra::registerUserType](@ref luwra::registerUserType).
	template <typename UserType> inline
	void registerUserType(
		const MemberMap& methods = MemberMap(),
		const MemberMap& meta_methods = MemberMap()
	) const {
		luwra::registerUserType<UserType>(state, methods, meta_methods);
	}

	/// See [luwra::push](@ref luwra::push).
	template <typename Type> inline
	void push(Type&& value) const {
		luwra::push(state, std::forward<Type>(value));
	}

	/// See [luwra::push](@ref luwra::push).
	template <typename First, typename Second, typename... Rest> inline
	void push(First&& first, Second&& second, Rest&&... rest) const {
		luwra::push(
			state,
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
	template <typename Type> inline
	Type read(int index) const {
		return luwra::read<Type>(state, index);
	}

	/// See [luwra::apply](@ref luwra::apply).
	template <typename Callable, typename... ExtraArgs> inline
	internal::ReturnTypeOf<Callable> apply(
		int            pos,
		Callable&&     func,
		ExtraArgs&&... args
	) const {
		return luwra::apply(
			state,
			pos,
			std::forward<Callable>(func),
			std::forward<ExtraArgs>(args)...
		);
	}

	/// See [luwra::map](@ref luwra::map).
	template <typename Callable, typename... ExtraArgs> inline
	size_t map(int pos, Callable&& func, ExtraArgs&&... args) const {
		return luwra::map(
			state,
			pos,
			std::forward<Callable>(func),
			std::forward<ExtraArgs>(args)...
		);
	}

	/// See [luwra::equal](@ref luwra::equal).
	bool equal(int index1, int index2) const {
		return luwra::equal(state, index1, index2);
	}

	/// See [luwra::setMetatable](@ref luwra::setMetatable).
	void setMetatable(const char* name) const {
		luwra::setMetatable(state, name);
	}

	/// Execute a piece of code.
	inline
	int runString(const char* code) const {
		return luaL_dostring(state, code);
	}

	/// Execute a file.
	inline
	int runFile(const char* filepath) const {
		return luaL_dofile(state, filepath);
	}
};

LUWRA_NS_END

#endif

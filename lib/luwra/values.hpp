/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_VALUES_H_
#define LUWRA_VALUES_H_

#include "common.hpp"
#include "internal/indexsequence.hpp"

#include <utility>
#include <string>

LUWRA_NS_BEGIN

template <typename UserType>
struct Value;

/// Alias for @ref Value<Type>
template <typename Type>
struct Value<const Type>: Value<Type> {};

/// Alias for @ref Value<Type>
template <typename Type>
struct Value<volatile Type>: Value<Type> {};

/// Alias for @ref Value<Type>
template <typename Type>
struct Value<const volatile Type>: Value<Type> {};

/// Alias for @ref Value<Type>
template <typename Type>
struct Value<Type&>: Value<Type> {};

/// Alias for @ref Value<Type>
template <typename Type>
struct Value<Type&&>: Value<Type> {};

namespace internal {
	struct InferValueType {
		State* state;
		int index;

		template <typename Type> inline
		operator Type() const && {
			return Value<Type>::read(state, index);
		}

		template <typename Type> inline
		operator Type&() const & {
			return Value<Type&>::read(state, index);
		}
	};
}

/// Enables reading of type-infered values
template <>
struct Value<internal::InferValueType> {
	static inline
	const internal::InferValueType read(State* state, int index) {
		return {state, index};
	}
};

/// Enables reading/pushing `nil`
template <>
struct Value<std::nullptr_t> {
	static inline
	std::nullptr_t read(State* state, int n) {
		luaL_checktype(state, n, LUA_TNIL);
		return nullptr;
	}

	static inline
	void push(State* state, std::nullptr_t) {
		lua_pushnil(state);
	}
};

/// Enables reading Lua threads
template <>
struct Value<State*> {
	static inline
	State* read(State* state, int n) {
		luaL_checktype(state, n, LUA_TTHREAD);
		return lua_tothread(state, n);
	}
};

/// Enables reading/pushing light data
template <>
struct Value<void*> {
	static inline
	void* read(State* state, int n) {
		luaL_checktype(state, n, LUA_TLIGHTUSERDATA);
		return lua_touserdata(state, n);
	}

	static inline
	void push(State* state, void* data) {
		lua_pushlightuserdata(state, data);
	}
};

/// Enables reading constant light data
template <>
struct Value<const void*> {
	static inline
	const void* read(State* state, int n) {
		luaL_checktype(state, n, LUA_TLIGHTUSERDATA);
		return lua_touserdata(state, n);
	}
};

namespace internal {
	template <typename Type>
	struct NumericTransportValue {
		static_assert(
			sizeof(Type) == -1,
			"Parameter to NumericTransportValue is not a numeric base type"
		);
	};

	// Transport unit `Integer`
	template <>
	struct NumericTransportValue<Integer> {
		static inline
		Integer read(State* state, int index) {
			// We have to copy the value at the given index because luaL_checkinteger might change
			// the value.
			lua_pushvalue(state, index);
			Integer ret = luaL_checkinteger(state, -1);
			lua_pop(state, 1);
			return ret;
		}

		static inline
		void push(State* state, Integer value) {
			lua_pushinteger(state, value);
		}
	};

	// Transport unit `Number`
	template <>
	struct NumericTransportValue<Number> {
		static inline
		Number read(State* state, int index) {
			// We have to copy the value at the given index because luaL_checknumber might change
			// the value.
			lua_pushvalue(state, index);
			Number ret = luaL_checknumber(state, -1);
			lua_pop(state, 1);
			return ret;
		}

		static inline
		void push(State* state, Number value) {
			lua_pushnumber(state, value);
		}
	};

	// Base for `Value<Type>` specializations which uses `Transport` as transport unit
	template <typename Type, typename Transport>
	struct NumericValueBase {
		static inline
		Type read(State* state, int index) {
			return static_cast<Type>(NumericTransportValue<Transport>::read(state, index));
		}

		static inline
		void push(State* state, Type value) {
			NumericTransportValue<Transport>::push(state, static_cast<Transport>(value));
		}
	};
}

// Define an integral type which will be transported via `base`.
#define LUWRA_DEF_NUMERIC(base, type) \
	template <> \
	struct Value<type>: internal::NumericValueBase<type, base> {};

// Lua-dependent types
LUWRA_DEF_NUMERIC(Number, float)
LUWRA_DEF_NUMERIC(Number, double)
LUWRA_DEF_NUMERIC(Number, long double)

// Integral types
LUWRA_DEF_NUMERIC(Integer, signed   char)
LUWRA_DEF_NUMERIC(Integer, unsigned char)
LUWRA_DEF_NUMERIC(Integer, signed   short)
LUWRA_DEF_NUMERIC(Integer, unsigned short)
LUWRA_DEF_NUMERIC(Integer, signed   int)
LUWRA_DEF_NUMERIC(Integer, unsigned int)
LUWRA_DEF_NUMERIC(Integer, signed   long int)
LUWRA_DEF_NUMERIC(Integer, unsigned long int)
LUWRA_DEF_NUMERIC(Integer, signed   long long int)
LUWRA_DEF_NUMERIC(Integer, unsigned long long int)

// Do not export this macros
#undef LUWRA_DEF_NUMERIC

/// Enables reading/pushing strings as C strings
template <>
struct Value<const char*> {
	static inline
	const char* read(State* state, int index) {
		// We have to copy the value at the given index because luaL_checkstring might change it.
		lua_pushvalue(state, index);
		const char* ret = luaL_checkstring(state, -1);
		lua_pop(state, 1);
		return ret;
	}

	static inline
	void push(State* state, const char* value) {
		lua_pushstring(state, value);
	}
};

/// Enables reading/pushing string as `std::string`.
template <>
struct Value<std::string> {
	static inline
	std::string read(State* state, int index) {
		// We have to copy the value at the given index because luaL_checklstring might change it.
		lua_pushvalue(state, index);
		size_t length;
		const char* value = luaL_checklstring(state, -1, &length);
		lua_pop(state, 1);
		return {value, length};
	}

	static inline
	void push(State* state, const std::string& value) {
		lua_pushstring(state, value.c_str());
	}
};

/// Enables reading/pushing booleans
template <>
struct Value<bool> {
	static inline
	bool read(State* state, int n) {
		luaL_checktype(state, n, LUA_TBOOLEAN);
		return lua_toboolean(state, n) == 1;
	}

	static inline
	void push(State* state, bool value) {
		lua_pushboolean(state, static_cast<int>(value));
	}
};

/// Indicates the type of a Lua value
enum class LuaType {
	Nil = LUA_TNIL,
	Number = LUA_TNUMBER,
	Boolean = LUA_TBOOLEAN,
	String = LUA_TSTRING,
	Table = LUA_TTABLE,
	Function = LUA_TFUNCTION,
	Userdata = LUA_TUSERDATA,
	LightUserdata = LUA_TLIGHTUSERDATA,
	Thread = LUA_TTHREAD
};

/// Enables reading the type of a value.
template<>
struct Value<LuaType> {
	static inline
	LuaType read(State* state, int n) {
		return static_cast<LuaType>(lua_type(state, n));
	}
};

/// Alias for `Value<const char*>`
template <>
struct Value<char*>: Value<const char*> {};

/// Alias for `Value<const char*>`
template <size_t N>
struct Value<char[N]>: Value<const char*> {};

/// Alias for `Value<const char*>`
template <size_t N>
struct Value<const char[N]>: Value<const char*> {};

/// A version of `Value` for pushing return values onto the stack. `ReturnValue` inherits
/// `push` implementations from `Value`.
template <typename Type>
struct ReturnValue {
	template <typename... Args> static inline
	size_t push(State* state, Args&&... args) {
		Value<Type>::push(state, std::forward<Args>(args)...);
		return 1;
	}
};

/// Alias for `ReturnValue<Type>`
template <typename Type>
struct ReturnValue<const Type>: ReturnValue<Type> {};

/// Alias for `ReturnValue<Type>`
template <typename Type>
struct ReturnValue<volatile Type>: ReturnValue<Type> {};

/// Alias for `ReturnValue<Type>`
template <typename Type>
struct ReturnValue<const volatile Type>: ReturnValue<Type> {};

/// Alias for `ReturnValue<Type>`
template <typename Type>
struct ReturnValue<Type&>: ReturnValue<Type> {};

/// Alias for `ReturnValue<Type>`
template <typename Type>
struct ReturnValue<Type&&>: ReturnValue<Type> {};

LUWRA_NS_END

#endif

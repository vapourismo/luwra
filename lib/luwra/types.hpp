/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_TYPES_H_
#define LUWRA_TYPES_H_

#include "common.hpp"
#include "compat.hpp"

#include <utility>
#include <tuple>
#include <string>
#include <type_traits>
#include <memory>
#include <limits>
#include <vector>
#include <list>
#include <initializer_list>
#include <map>

LUWRA_NS_BEGIN

/* Lua types */
using Integer = lua_Integer;
using Number = lua_Number;
using State = lua_State;
using CFunction = lua_CFunction;

/**
 * User type
 */
template <typename T>
struct Value;

// Nil
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

template <>
struct Value<State*> {
	static inline
	State* read(State* state, int n) {
		luaL_checktype(state, n, LUA_TTHREAD);
		return lua_tothread(state, n);
	}
};

/**
 * Convenient wrapped for [Value<T>::push](@ref Value<T>::push).
 */
template <typename T> static inline
void push(State* state, T&& value) {
	Value<T>::push(state, std::forward<T>(value));
}

/**
 * Allows you to push multiple values at once.
 */
template <typename T1, typename T2, typename... TR> static inline
void push(State* state, T1&& value, T2&& head, TR&&... rest) {
	push(state, std::forward<T1>(value));
	push(state, std::forward<T2>(head), std::forward<TR>(rest)...);
}

/**
 * Convenient wrapper for [Value<T>::read](@ref Value<T>::read).
 */
template <typename T> static inline
T read(State* state, int index) {
	return Value<T>::read(state, index);
}

namespace internal {
	template <typename T>
	struct NumericTransportValue {
		static_assert(
			sizeof(T) == -1,
			"Parameter to NumericTransportValue is not a numeric base type"
		);
	};

	// Transport unit `Integer`
	template <>
	struct NumericTransportValue<Integer> {
		static inline
		Integer read(State* state, int index) {
			return luaL_checkinteger(state, index);
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
			return luaL_checknumber(state, index);
		}

		static inline
		void push(State* state, Number value) {
			lua_pushnumber(state, value);
		}
	};

	// Base for `Value<I>` specializations which uses `B` as transport unit
	template <typename I, typename B>
	struct NumericValueBase {
		static inline
		I read(State* state, int index) {
			return static_cast<I>(NumericTransportValue<B>::read(state, index));
		}

		static inline
		void push(State* state, I value) {
			NumericTransportValue<B>::push(state, static_cast<B>(value));
		}
	};
}

/**
 * Define an integral type which will be transported via `base`.
 */
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

// Do not export these macros
#undef LUWRA_DEF_NUMERIC

template <>
struct Value<const char*> {
	static inline
	const char* read(State* state, int n) {
		return luaL_checkstring(state, n);
	}

	static inline
	void push(State* state, const char* value) {
		lua_pushstring(state, value);
	}
};

template <>
struct Value<std::string> {
	static inline
	std::string read(State* state, int n) {
		size_t length;
		const char* value = luaL_checklstring(state, n, &length);
		return {value, length};
	}

	static inline
	void push(State* state, const std::string& value) {
		lua_pushstring(state, value.c_str());
	}
};

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

// Alias for string literals
template <size_t n>
struct Value<char[n]>: Value<const char*> {};

// Alias for const string literals
template <size_t n>
struct Value<const char[n]>: Value<const char*> {};

namespace internal {
	// Create reference the value pointed to by `index`. Does not remove the referenced value.
	static inline
	int referenceValue(State* state, int index) {
		lua_pushvalue(state, index);
		return luaL_ref(state, LUA_REGISTRYINDEX);
	}

	// Implementation of a reference which takes care of the lifetime of a Lua reference
	struct ReferenceImpl {
		State* state;
		int ref;
		bool autoUnref = true;

		// Reference a value at an index.
		inline
		ReferenceImpl(State* state, int indexOrRef, bool isIndex = true):
			state(state),
			ref(isIndex ? referenceValue(state, indexOrRef) : indexOrRef),
			autoUnref(isIndex)
		{}

		// Reference the value on top of stack.
		inline
		ReferenceImpl(State* state):
			state(state),
			ref(luaL_ref(state, LUA_REGISTRYINDEX))
		{}

		// A (smart) pointer to an instance may be copied and moved, but the instance itself must
		// not be copied or moved. This allows us to have only one instance of `ReferenceImpl` per
		// Lua reference.
		ReferenceImpl(const ReferenceImpl& other) = delete;
		ReferenceImpl(ReferenceImpl&& other) = delete;
		ReferenceImpl& operator =(const ReferenceImpl&) = delete;
		ReferenceImpl& operator =(ReferenceImpl&&) = delete;

		inline
		~ReferenceImpl() {
			if (ref >= 0 && autoUnref) luaL_unref(state, LUA_REGISTRYINDEX, ref);
		}

		// Small shortcut to make the `push`-implementations for `Table` and `Reference` consistent,
		// since both use this struct internally.
		inline
		void push(State* targetState) const {
			lua_rawgeti(state, LUA_REGISTRYINDEX, ref);

			if (state != targetState)
				lua_xmove(state, targetState, 1);
		}

		inline
		void push() const {
			lua_rawgeti(state, LUA_REGISTRYINDEX, ref);
		}
	};

	using SharedReferenceImpl = std::shared_ptr<const internal::ReferenceImpl>;
}

/**
 * Reference to an arbitrary value.
 */
struct Reference {
	const internal::SharedReferenceImpl impl;

	/**
	 * Create a reference to the value at the given index.
	 */
	inline
	Reference(State* state, int indexOrRef = -1, bool isIndex = true):
		impl(std::make_shared<internal::ReferenceImpl>(state, indexOrRef, isIndex))
	{}

	/**
	 * Read the referenced value.
	 */
	template <typename T> inline
	T read() const {
		impl->push();
		T ret = Value<T>::read(impl->state, -1);

		lua_pop(impl->state, 1);
		return ret;
	}

	/**
	 * Shortcut for `read<T>()`.
	 */
	template <typename T> inline
	operator T() const {
		return read<T>();
	}
};

/**
 * See [Reference](@ref Reference).
 */
template <>
struct Value<Reference> {
	static inline
	Reference read(State* state, int index) {
		return {state, index, true};
	}

	static inline
	void push(State* state, const Reference& value) {
		value.impl->push(state);
	}
};

/**
 * Fix specialization for const types.
 */
template <typename T>
struct Value<const T>: Value<T> {};

/**
 * Fix specialization for volatile types.
 */
template <typename T>
struct Value<volatile T>: Value<T> {};

/**
 * Fix specialization for const volatile types.
 */
template <typename T>
struct Value<const volatile T>: Value<T> {};

/**
 * Fix specialization for lvalue reference types.
 */
template <typename U>
struct Value<U&>: Value<U> {};

/**
 * Fix specialization for rvalue reference types.
 */
template <typename U>
struct Value<U&&>: Value<U> {};

namespace internal {
	struct PushableI {
		virtual
		void push(State* state) const = 0;
	};

	template <typename T>
	struct PushableT: virtual PushableI {
		T value;

		template <typename P> inline
		PushableT(P&& value): value(std::forward<P>(value)) {}

		virtual
		void push(State* state) const {
			luwra::push(state, value);
		}
	};
}

/**
 * A value which may be pushed onto the stack.
 */
struct Pushable {
	std::shared_ptr<internal::PushableI> interface;

	template <typename T> inline
	Pushable(T&& value):
		interface(new internal::PushableT<T>(std::forward<T>(value)))
	{}

	inline
	bool operator <(const Pushable& other) const {
		return interface < other.interface;
	}
};

template <>
struct Value<Pushable> {
	static inline
	void push(State* state, const Pushable& value) {
		value.interface->push(state);
	}
};

template <typename T>
struct Value<std::vector<T>> {
	static inline
	void push(State* state, const std::vector<T>& vec) {
		lua_createtable(state, vec.size(), 0);

		int size = static_cast<int>(vec.size());
		for (int i = 0; i < size; i++) {
			luwra::push(state, vec[i]);
			lua_rawseti(state, -2, i + 1);
		}
	}
};

template <typename T>
struct Value<std::list<T>> {
	static inline
	void push(State* state, const std::list<T>& lst) {
		lua_createtable(state, lst.size(), 0);

		int i = 0;
		for (const T& item: lst) {
			luwra::push(state, item);
			lua_rawseti(state, -2, ++i);
		}
	}
};

template <typename K, typename V>
struct Value<std::map<K, V>> {
	static inline
	void push(State* state, const std::map<K, V>& map) {
		lua_createtable(state, 0, map.size());

		for (const auto& entry: map) {
			luwra::push(state, entry.first);
			luwra::push(state, entry.second);
			lua_rawset(state, -3);
		}
	}
};

LUWRA_NS_END

#endif

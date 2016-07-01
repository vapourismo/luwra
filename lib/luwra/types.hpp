/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_TYPES_H_
#define LUWRA_TYPES_H_

#include "common.hpp"
#include "internal/indexsequence.hpp"

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
template <typename UserType>
struct Value;

/**
 * Fix specialization for const types.
 */
template <typename Type>
struct Value<const Type>: Value<Type> {};

/**
 * Fix specialization for volatile types.
 */
template <typename Type>
struct Value<volatile Type>: Value<Type> {};

/**
 * Fix specialization for const volatile types.
 */
template <typename Type>
struct Value<const volatile Type>: Value<Type> {};

/**
 * Fix specialization for lvalue reference types.
 */
template <typename Type>
struct Value<Type&>: Value<Type> {};

/**
 * Fix specialization for rvalue reference types.
 */
template <typename Type>
struct Value<Type&&>: Value<Type> {};

/**
 * Convenient wrapped for [Value<Type>::push](@ref Value<Type>::push).
 */
template <typename Type> static inline
void push(State* state, Type&& value) {
	Value<Type>::push(state, std::forward<Type>(value));
}

/**
 * Allows you to push multiple values at once.
 */
template <typename First, typename Second, typename... Rest> static inline
void push(State* state, First&& value, Second&& head, Rest&&... rest) {
	push(state, std::forward<First>(value));
	push(state, std::forward<Second>(head), std::forward<Rest>(rest)...);
}

/**
 * Convenient wrapper for [Value<Type>::read](@ref Value<Type>::read).
 */
template <typename Type> static inline
auto read(State* state, int index) -> decltype(Value<Type>::read(state, index)) {
	return Value<Type>::read(state, index);
}

/**
 * Nil
 */
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

/**
 * Lua thread
 */
template <>
struct Value<State*> {
	static inline
	State* read(State* state, int n) {
		luaL_checktype(state, n, LUA_TTHREAD);
		return lua_tothread(state, n);
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
	template <typename Type> inline
	Type read() const {
		impl->push();
		Type ret = Value<Type>::read(impl->state, -1);

		lua_pop(impl->state, 1);
		return ret;
	}

	/**
	 * Shortcut for `read<Type>()`.
	 */
	template <typename Type> inline
	operator Type() const {
		return read<Type>();
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

namespace internal {
	struct PushableI {
		virtual
		void push(State* state) const = 0;
	};

	template <typename Type>
	struct PushableT: virtual PushableI {
		Type value;

		template <typename Source> inline
		PushableT(Source&& value): value(std::forward<Source>(value)) {}

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

	template <typename Type> inline
	Pushable(Type&& value):
		interface(new internal::PushableT<Type>(std::forward<Type>(value)))
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

template <typename Type>
struct Value<std::vector<Type>> {
	static inline
	void push(State* state, const std::vector<Type>& vec) {
		lua_createtable(state, vec.size(), 0);

		int size = static_cast<int>(vec.size());
		for (int i = 0; i < size; i++) {
			luwra::push(state, vec[i]);
			lua_rawseti(state, -2, i + 1);
		}
	}
};

template <typename Type>
struct Value<std::list<Type>> {
	static inline
	void push(State* state, const std::list<Type>& lst) {
		lua_createtable(state, lst.size(), 0);

		int i = 0;
		for (const Type& item: lst) {
			luwra::push(state, item);
			lua_rawseti(state, -2, ++i);
		}
	}
};

template <typename Key, typename Type>
struct Value<std::map<Key, Type>> {
	static inline
	void push(State* state, const std::map<Key, Type>& map) {
		lua_createtable(state, 0, map.size());

		for (const auto& entry: map) {
			luwra::push(state, entry.first);
			luwra::push(state, entry.second);
			lua_rawset(state, -3);
		}
	}
};

template <typename Type>
struct ReturnValue {
	template <typename... Args> static inline
	size_t push(State* state, Args&&... args) {
		Value<Type>::push(state, std::forward<Args>(args)...);
		return 1;
	}
};

/**
 * Fix specialization for const types.
 */
template <typename Type>
struct ReturnValue<const Type>: ReturnValue<Type> {};

/**
 * Fix specialization for volatile types.
 */
template <typename Type>
struct ReturnValue<volatile Type>: ReturnValue<Type> {};

/**
 * Fix specialization for const volatile types.
 */
template <typename Type>
struct ReturnValue<const volatile Type>: ReturnValue<Type> {};

/**
 * Fix specialization for lvalue reference types.
 */
template <typename Type>
struct ReturnValue<Type&>: ReturnValue<Type> {};

/**
 * Fix specialization for rvalue reference types.
 */
template <typename Type>
struct ReturnValue<Type&&>: ReturnValue<Type> {};

/**
 * Push a return value onto the stack.
 *
 * \param state Lua state
 * \param value Return value
 * \returns Number of Lua values that have been pushed onto the stack
 */
template <typename Type> inline
size_t pushReturn(State* state, Type&& value) {
	return ReturnValue<Type>::push(state, std::forward<Type>(value));
}

/**
 * Push multiple return values onto the stack.
 *
 * \param state   Lua state
 * \param first   First return value
 * \param second  Second return value
 * \param rest    More return values
 * \returns Number of Lua values that have been pushed onto the stack
 */
template <typename First, typename Second, typename... Rest> inline
size_t pushReturn(State* state, First&& first, Second&& second, Rest&&... rest) {
	return
		pushReturn(state, std::forward<First>(first)) +
		pushReturn(state, std::forward<Second>(second), std::forward<Rest>(rest)...);
}

namespace internal {
	template <typename Seq, typename...>
	struct _TuplePusher {
		static_assert(
			sizeof(Seq) == -1,
			"Invalid template parameters to _TuplePusher"
		);
	};

	template <size_t... Indices, typename... Contents>
	struct _TuplePusher<IndexSequence<Indices...>, Contents...> {
		static inline
		size_t push(State* state, const std::tuple<Contents...>& value) {
			return pushReturn(state, std::get<Indices>(value)...);
		}
	};

	template <typename... Contents>
	using TuplePusher = _TuplePusher<MakeIndexSequence<sizeof...(Contents)>, Contents...>;
}

template <typename... Contents>
struct ReturnValue<std::tuple<Contents...>>:
	internal::TuplePusher<Contents...> {};

LUWRA_NS_END

#endif

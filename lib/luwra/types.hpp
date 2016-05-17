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
struct Value: Value<T&> {};

// Nil
template <>
struct Value<std::nullptr_t> {
	static inline
	std::nullptr_t read(State* state, int n) {
		luaL_checktype(state, n, LUA_TNIL);
		return nullptr;
	}

	static inline
	size_t push(State* state, std::nullptr_t) {
		lua_pushnil(state);
		return 1;
	}
};
/**
 * Convenient wrapped for [Value<T>::push](@ref Value<T>::push).
 */
template <typename T> static inline
size_t push(State* state, T&& value) {
	using U = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
	return Value<U>::push(state, std::forward<T>(value));
}

/**
 * Allows you to push multiple values at once.
 */
template <typename T1, typename T2, typename... TR>
size_t push(State* state, T1&& value, T2&& head, TR&&... rest) {
	return
		push(state, std::forward<T1>(value)) +
		push(state, std::forward<T2>(head), std::forward<TR>(rest)...);
}

/**
 * Convenient wrapper for [Value<T>::read](@ref Value<T>::read).
 */
template <typename T> static inline
T read(State* state, int index) {
	return Value<T>::read(state, index);
}

/**
 * Define a template specialization of `Value` for `type` with a `retrf(State*, int)` which
 * extracts it from the stack and a `pushf(State*, type)` which pushes the value on the stack again.
 * This assumes that only one value will be pushed onto the stack.
 */
#define LUWRA_DEF_VALUE(type, retrf, pushf)             \
	template <>                                         \
	struct Value<type> {                                \
		static inline                                   \
		type read(State* state, int n) {                \
			return static_cast<type>(retrf(state, n));  \
		}                                               \
		                                                \
		static inline                                   \
		size_t push(State* state, type value) {         \
			pushf(state, value);                        \
			return 1;                                   \
		}                                               \
	}

#ifndef luaL_checkboolean
	/**
	 * Check if the value at index `n` is a boolean and retrieve its value.
	 */
	#define luaL_checkboolean(state, n) \
		(luaL_checktype(state, n, LUA_TBOOLEAN), lua_toboolean(state, n))
#endif

#ifndef luaL_checkcfunction
	/**
	 * Check if the value at index `n` is a C function and retrieve it.
	 */
	#define luaL_checkcfunction(state, n) \
		(luaL_checktype(state, n, LUA_TCFUNCTION), lua_tocfunction(state, n))
#endif

#ifndef luaL_pushstdstring
	/**
	 * push a `std::string` as string onto the stack.
	 */
	#define luaL_pushstdstring(state, stdstring) \
		(lua_pushstring(state, (stdstring).c_str()))
#endif

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
		size_t push(State* state, Integer value) {
			lua_pushinteger(state, value);
			return 1;
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
		size_t push(State* state, Number value) {
			lua_pushnumber(state, value);
			return 1;
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
		size_t push(State* state, I value) {
			NumericTransportValue<B>::push(state, static_cast<B>(value));
			return 1;
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

// C/C++ types
LUWRA_DEF_VALUE(const char*, luaL_checkstring,  lua_pushstring);
LUWRA_DEF_VALUE(std::string, luaL_checkstring,  luaL_pushstdstring);

// Do not export these macros
#undef LUWRA_DEF_VALUE
#undef LUWRA_DEF_NUMERIC

template <>
struct Value<bool> {
	static inline
	bool read(State* state, int n) {
		return luaL_checkboolean(state, n) == 1;
	}

	static inline
	size_t push(State* state, bool value) {
		lua_pushboolean(state, static_cast<int>(value));
		return 1;
	}
};

// Alias for string literals
template <size_t n>
struct Value<char[n]>: Value<const char*> {};

// Alias for const string literals
template <size_t n>
struct Value<const char[n]>: Value<const char*> {};

/**
 * C Functions may be pushed aswell.
 */
template <>
struct Value<CFunction> {
	static inline
	size_t push(State* state, CFunction fun) {
		lua_pushcfunction(state, fun);
		return 1;
	}
};

namespace internal {
	// Create reference the value pointed to by `index`. Does not remove the referenced value.
	static inline
	int referenceValue(State* state, int index) {
		lua_pushvalue(state, index);
		return luaL_ref(state, LUA_REGISTRYINDEX);
	}

	// Implementation of a reference which takes care of the lifetime of a Lua reference
	struct ReferenceImpl {
		State* const state;
		const int ref;
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

		inline
		~ReferenceImpl() {
			if (ref >= 0 && autoUnref) luaL_unref(state, LUA_REGISTRYINDEX, ref);
		}

		// Small shortcut to make the `push`-implementations for `Table` and `Reference` consistent,
		// since both use this struct internally.
		inline
		size_t push(State* targetState) {
			lua_rawgeti(state, LUA_REGISTRYINDEX, ref);

			if (state != targetState)
				lua_xmove(state, targetState, 1);

			return 1;
		}

		inline
		size_t push() {
			lua_rawgeti(state, LUA_REGISTRYINDEX, ref);
			return 1;
		}
	};

	using SharedReferenceImpl = std::shared_ptr<internal::ReferenceImpl>;
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
	Reference(State* state, int indexOrRef, bool isIndex = true):
		impl(std::make_shared<internal::ReferenceImpl>(state, indexOrRef, isIndex))
	{}

	/**
	 * Create a reference to the value at the top of the stack.
	 */
	inline
	Reference(State* state):
		impl(std::make_shared<internal::ReferenceImpl>(state))
	{}

	/**
	 * Read the referenced value.
	 */
	template <typename T> inline
	T read() {
		size_t pushed = impl->push();
		T ret = Value<T>::read(impl->state, -1);

		lua_pop(impl->state, pushed);
		return ret;
	}

	/**
	 * Shortcut for `read<T>()`.
	 */
	template <typename T> inline
	operator T() {
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
	size_t push(State* state, const Reference& value) {
		return value.impl->push(state);
	}
};

namespace internal {
	template <typename>
	struct StackPusher;

	template <size_t I>
	struct StackPusher<IndexSequence<I>> {
		template <typename... T> static inline
		size_t push(State* state, const std::tuple<T...>& package) {
			return luwra::push(state, std::get<I>(package));
		}
	};

	template <size_t I, size_t... Is>
	struct StackPusher<IndexSequence<I, Is...>> {
		template <typename... T> static inline
		size_t push(State* state, const std::tuple<T...>& package) {
			return
				StackPusher<IndexSequence<I>>::push(state, package) +
				StackPusher<IndexSequence<Is...>>::push(state, package);
		}
	};
}

/**
 * Allows you to use multiple return values.
 */
template <typename... A>
struct Value<std::tuple<A...>> {
	static inline
	size_t push(State* state, const std::tuple<A...>& value) {
		using Seq = internal::MakeIndexSequence<sizeof...(A)>;
		return internal::StackPusher<Seq>::push(state, value);
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

namespace internal {
	struct PushableI {
		virtual
		size_t push(State* state) const = 0;

		virtual
		PushableI* copy() const = 0;

		virtual
		~PushableI() {}
	};

	template <typename T>
	struct PushableT: virtual PushableI {
		T value;

		inline
		PushableT(T value): value(value) {}

		virtual
		size_t push(State* state) const {
			return luwra::push(state, value);
		}

		virtual
		PushableI* copy() const {
			return new PushableT<T>(value);
		}
	};
}

/**
 * A value which may be pushed onto the stack.
 */
struct Pushable: virtual internal::PushableI {
	internal::PushableI* interface;

	template <typename T> inline
	Pushable(T value): interface(new internal::PushableT<T>(value)) {}

	inline
	Pushable(Pushable&& other): interface(other.interface) {
		other.interface = nullptr;
	}

	Pushable(const Pushable& other): interface(other.interface->copy()) {}

	virtual
	size_t push(State* state) const {
		return interface->push(state);
	}

	virtual
	internal::PushableI* copy() const {
		return new Pushable(*this);
	}

	virtual
	~Pushable() {
		if (interface)
			delete interface;
	}
};

template <>
struct Value<Pushable> {
	static inline
	size_t push(State* state, const Pushable& value) {
		return value.push(state);
	}
};

LUWRA_NS_END

#endif

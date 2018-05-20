/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_TYPES_PUSHABLE_H_
#define LUWRA_TYPES_PUSHABLE_H_

#include "../common.hpp"
#include "../values.hpp"
#include "../stack.hpp"

#include <utility>
#include <memory>

LUWRA_NS_BEGIN

namespace internal {
	struct PushableI {
		virtual
		void push(State* state) const = 0;

		virtual ~PushableI() {}
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

		virtual ~PushableT() {}
	};

	using SharedPushableImpl = std::shared_ptr<PushableI>;
}

/// Arbitrary pushable value
///
/// This class is implicitly constructible using any type. One can use this class with STL
/// containers in order to achieve pushable mixed-type containers.
struct Pushable {
	const internal::SharedPushableImpl interface;

	template <typename Type> inline
	Pushable(Type&& value):
		interface(new internal::PushableT<Type>(std::forward<Type>(value)))
	{}

	// Used in ordered STL containers
	inline
	bool operator <(const Pushable& other) const {
		return interface < other.interface;
	}
};

/// Enables pushing for `Pushables`
template <>
struct Value<Pushable> {
	static inline
	void push(State* state, const Pushable& value) {
		value.interface->push(state);
	}
};

LUWRA_NS_END

#endif

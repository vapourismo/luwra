/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2015, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_FIELDS_H_
#define LUWRA_FIELDS_H_

#include "common.hpp"
#include "types.hpp"

LUWRA_NS_BEGIN

namespace internal {
	/**
	 * Helper struct for wrapping user type fields
	 */
	template <typename T>
	struct FieldWrapper {
		static_assert(
			sizeof(T) == -1,
			"Parameter to FieldWrapper is not a property signature"
		);
	};

	template <typename T, typename R>
	struct FieldWrapper<R T::*> {
		template <R T::* field_pointer> static inline
		int invoke(State* state) {
			if (lua_gettop(state) > 1) {
				read<T*>(state, 1)->*field_pointer = read<R>(state, 2);
				return 0;
			} else {
				return static_cast<int>(push(state, read<T*>(state, 1)->*field_pointer));
			}
		}
	};

	template <typename T, typename R>
	struct FieldWrapper<const R T::*> {
		template <const R T::* field_pointer> static inline
		int invoke(State* state) {
			return static_cast<int>(push(state, read<T*>(state, 1)->*field_pointer));
		}
	};
}

LUWRA_NS_END

/**
 * Generate a `lua_CFunction` get/set wrapper for a property accessor.
 * \param field Fully qualified property name (Do not supply a pointer)
 * \return Wrapped function as `lua_CFunction`
 */
#define LUWRA_WRAP_FIELD(field) \
	(&luwra::internal::FieldWrapper<decltype(&field)>::invoke<&field>)

#endif

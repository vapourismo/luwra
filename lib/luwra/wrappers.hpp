/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_WRAPPERS_H_
#define LUWRA_WRAPPERS_H_

#include "common.hpp"
#include "values.hpp"
#include "stack.hpp"
#include "internal/types.hpp"

#include <utility>
#include <type_traits>

LUWRA_NS_BEGIN

namespace internal {
	// Method wrapper implementation for calling a method of type MethodPointer on an instance of
	// Klass.
	template <
		typename MethodPointer,
		typename Klass = typename MemberInfo<MethodPointer>::MemberOf
	>
	struct MemberMethodWrapperImpl {
		using BaseKlass = typename MemberInfo<MethodPointer>::MemberOf;

		// Make sure that a member pointer with type MethodPointer is member of a base class that
		// Klass derives from. This condition also allows BaseKlass == Klass.
		static_assert(
			std::is_base_of<BaseKlass, Klass>::value,
			"Instances of MethodPointer are not part of Klass"
		);

		template <typename... Args>
		struct _ImplementerNonVoid {
			template <size_t... Indices>
			struct SeqReceiver {
				template <MethodPointer meth> static inline
				int invoke(State* state) {
					return static_cast<int>(
						pushReturn(
							state,
							(read<Klass*>(state, 1)->*meth)(read<Args>(state, 2 + Indices)...)
						)
					);
				}
			};
		};

		template <typename... Args>
		struct _ImplementerVoid {
			template <size_t... Indices>
			struct SeqReceiver {
				template <MethodPointer meth> static inline
				int invoke(State* state) {
					(read<Klass*>(state, 1)->*meth)(read<Args>(state, 2 + Indices)...);
					return 0;
				}
			};
		};

		template <typename... Types>
		using _Implementer =
			typename std::conditional<
				std::is_same<ReturnTypeOf<MethodPointer>, void>::value,
				_ImplementerVoid<Types...>,
				_ImplementerNonVoid<Types...>
			>::type;

		template <typename... Types>
		using Implementer =
			typename MakeIndexSequence<sizeof...(Types)>::template Relay<
				_Implementer<Types...>::template SeqReceiver
			>;

		using Implementation =
			typename ArgumentsOf<MethodPointer>::template Relay<Implementer>;
	};

	// Catch attempts to wrap non-member pointers.
	template <
		typename MemberPointer,
		typename Klass = typename MemberInfo<MemberPointer>::MemberOf
	>
	struct MemberWrapper {
		static_assert(
			sizeof(MemberPointer) == -1,
			"MemberPointer is not a member pointer type"
		);
	};

	// Wrap methods that expect 'this' to be const-volatile-qualified.
	template <typename Klass, typename BaseKlass, typename Ret, typename... Args>
	struct MemberWrapper<Ret (BaseKlass::*)(Args...) const volatile, Klass>:
		MemberMethodWrapperImpl<Ret (BaseKlass::*)(Args...) const volatile, Klass>::Implementation {};

	// Wrap methods that expect 'this' to be const-qualified.
	template <typename Klass, typename BaseKlass, typename Ret, typename... Args>
	struct MemberWrapper<Ret (BaseKlass::*)(Args...) const, Klass>:
		MemberMethodWrapperImpl<Ret (BaseKlass::*)(Args...) const, Klass>::Implementation {};

	// Wrap methods that expect 'this' to be volatile-qualified.
	template <typename Klass, typename BaseKlass, typename Ret, typename... Args>
	struct MemberWrapper<Ret (BaseKlass::*)(Args...) volatile, Klass>:
		MemberMethodWrapperImpl<Ret (BaseKlass::*)(Args...) volatile, Klass>::Implementation {};

	// Wrap methods that expect 'this' to be unqualified.
	template <typename Klass, typename BaseKlass, typename Ret, typename... Args>
	struct MemberWrapper<Ret (BaseKlass::*)(Args...), Klass>:
		MemberMethodWrapperImpl<Ret (BaseKlass::*)(Args...), Klass>::Implementation {};

	// Wrap const-qualified field, provides only the getter.
	template <typename Klass, typename BaseKlass, typename FieldType>
	struct MemberWrapper<const FieldType BaseKlass::*, Klass> {
		// Make sure that a member pointer of the given type is member of a base class that Klass
		// derives from. The case BaseKlass == Klass is also accepted.
		static_assert(
			std::is_base_of<BaseKlass, Klass>::value,
			"Instances of the given field pointer type are not part of Klass"
		);

		template <const FieldType BaseKlass::* accessor> static inline
		int invoke(State* state) {
			push(state, read<Klass*>(state, 1)->*accessor);
			return 1;
		}
	};

	// Wrap field, provides getter and setter.
	template <typename Klass, typename BaseKlass, typename FieldType>
	struct MemberWrapper<FieldType BaseKlass::*, Klass> {
		// Make sure that a member pointer of the given type is member of a base class that Klass
		// derives from. The case BaseKlass == Klass is also accepted.
		static_assert(
			std::is_base_of<BaseKlass, Klass>::value,
			"Instances of the given field pointer type are not part of Klass"
		);

		template <FieldType BaseKlass::* accessor> static inline
		int invoke(State* state) {
			if (lua_gettop(state) > 1) {
				read<Klass*>(state, 1)->*accessor = read<FieldType>(state, 2);
				return 0;
			} else {
				push(state, read<Klass*>(state, 1)->*accessor);
				return 1;
			}
		}
	};

	template <typename Ret, typename... Args>
	struct FunctionWrapperImpl {
		template <size_t... Indices>
		struct _ImplementerNonVoid {
			template <Ret (* func)(Args...)> static inline
			int invoke(State* state) {
				return static_cast<int>(
					pushReturn(
						state,
						func(read<Args>(state, 1 + Indices)...)
					)
				);
			}
		};

		template <size_t... Indices>
		struct _ImplementerVoid {
			template <void (* func)(Args...)> static inline
			int invoke(State* state) {
				func(read<Args>(state, 1 + Indices)...);
				return 0;
			}
		};

		template <size_t... Indices>
		using Implementer =
			typename std::conditional<
				std::is_same<Ret, void>::value,
				_ImplementerVoid<Indices...>,
				_ImplementerNonVoid<Indices...>
			>::type;

		using Implementation =
			typename MakeIndexSequence<sizeof...(Args)>::template Relay<
				Implementer
			>;
	};

	// Catch attempts to wrap unwrappable types.
	template <typename ToBeWrapped>
	struct Wrapper {
		static_assert(
			sizeof(ToBeWrapped) == -1,
			"Template parameter to Wrapper is not wrappable"
		);
	};

	// Wrap a function. All parameters are read off the stack using their respective `Value`
	// specialization and subsequently passed to the function. The returned value will be pushed
	// onto the stack.
	template <typename Ret, typename... Args>
	struct Wrapper<Ret (Args...)>:
		FunctionWrapperImpl<Ret, Args...>::Implementation {};

	// An alias for the `Ret (Args...)` specialization. It primarily exists because functions aren't
	// passable as values, instead they are referenced using a function pointer.
	template <typename Ret, typename... Args>
	struct Wrapper<Ret (*)(Args...)>:
		Wrapper<Ret (Args...)> {};

	// Wrap methods that expect `this` to be 'const volatile'-qualified.
	template <typename Klasss, typename Ret, typename... Args>
	struct Wrapper<Ret (Klasss::*)(Args...) const volatile>:
		MemberWrapper<Ret (Klasss::*)(Args...) const volatile> {};

	// Wrap methods that expect `this` to be 'const'-qualified.
	template <typename Klasss, typename Ret, typename... Args>
	struct Wrapper<Ret (Klasss::*)(Args...) const>:
		MemberWrapper<Ret (Klasss::*)(Args...) const> {};

	// Wrap methods that expect `this` to be 'volatile'-qualified.
	template <typename Klasss, typename Ret, typename... Args>
	struct Wrapper<Ret (Klasss::*)(Args...) volatile>:
		MemberWrapper<Ret (Klasss::*)(Args...) volatile> {};

	// Wrap methods that expect `this` to be unqualified.
	template <typename Klasss, typename Ret, typename... Args>
	struct Wrapper<Ret (Klasss::*)(Args...)>:
		MemberWrapper<Ret (Klasss::*)(Args...)> {};

	// Wrap field.
	template <typename Klasss, typename Ret>
	struct Wrapper<Ret Klasss::*>:
		MemberWrapper<Ret Klasss::*> {};
}

LUWRA_NS_END

/// Generate a `lua_CFunction` wrapper for a field, method or function.
/// \returns Wrapped entity as `lua_CFunction`
#define LUWRA_WRAP(entity) \
	(&luwra::internal::Wrapper<decltype(&entity)>::template invoke<&entity>)

/// Same as `LUWRA_WRAP` but specifically for members of a class. It is imperative that you use this
/// macro instead of `LUWRA_WRAP` when working with inherited members.
#define LUWRA_WRAP_MEMBER(base, name) \
	(&luwra::internal::MemberWrapper<decltype(&__LUWRA_NS_RESOLVE(base, name)), base>::template invoke<&__LUWRA_NS_RESOLVE(base, name)>)

#endif

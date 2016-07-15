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
	struct MethodWrapperImpl {
		using BaseKlass = typename MemberInfo<MethodPointer>::MemberOf;

		// Make sure that a member pointer with type MethodPointer is member of a base class that
		// Klass derives from. This condition also allows BaseKlass == Klass.
		static_assert(
			std::is_base_of<BaseKlass, Klass>::value,
			"Instances of MethodPointer are not part of Klass"
		);

		// Implements 'invoke' for methods with a return value.
		template <typename... Args>
		struct ImplementationNonVoid {
			template <size_t... Indices>
			struct SeqReceiver {
				template <MethodPointer meth> static inline
				int invoke(State* state) {
					return static_cast<int>(
						pushReturn(
							state,
							// Read user type instance and resolve method.
							(read<Klass*>(state, 1)->*meth)(
								// Retrieve parameters from the stack and pass them to the method.
								read<Args>(state, 2 + Indices)...
							)
						)
					);
				}
			};
		};

		// Implements 'invoke' for methods without a return value.
		template <typename... Args>
		struct ImplementationVoid {
			template <size_t... Indices>
			struct SeqReceiver {
				template <MethodPointer meth> static inline
				int invoke(State* state) {
					// Read user type instance and resolve method.
					(read<Klass*>(state, 1)->*meth)(
						// Retrieve parameters from the stack and pass them to the method.
						read<Args>(state, 2 + Indices)...
					);

					return 0;
				}
			};
		};

		template <typename... Types>
		using ImplementationPicker =
			// Choose which implementation to use based on the return type of the given method.
			typename std::conditional<
				std::is_same<ReturnTypeOf<MethodPointer>, void>::value,
				ImplementationVoid<Types...>,
				ImplementationNonVoid<Types...>
			>::type;

		using Implementation =
			// Generate an index sequence based on the number of parameters. This sequence is
			// needed by SeqReceiver in order to efficiently unpack the parameters.
			typename MakeIndexSequence<ArgumentsOf<MethodPointer>::Length>::template Relay<
				// Relay a template parameter pack which contains the parameter types to
				// ImplementationPicker.
				ArgumentsOf<MethodPointer>::template Relay<
					ImplementationPicker
				>::template SeqReceiver
			>;
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
		MethodWrapperImpl<Ret (BaseKlass::*)(Args...) const volatile, Klass>::Implementation {};

	// Wrap methods that expect 'this' to be const-qualified.
	template <typename Klass, typename BaseKlass, typename Ret, typename... Args>
	struct MemberWrapper<Ret (BaseKlass::*)(Args...) const, Klass>:
		MethodWrapperImpl<Ret (BaseKlass::*)(Args...) const, Klass>::Implementation {};

	// Wrap methods that expect 'this' to be volatile-qualified.
	template <typename Klass, typename BaseKlass, typename Ret, typename... Args>
	struct MemberWrapper<Ret (BaseKlass::*)(Args...) volatile, Klass>:
		MethodWrapperImpl<Ret (BaseKlass::*)(Args...) volatile, Klass>::Implementation {};

	// Wrap methods that expect 'this' to be unqualified.
	template <typename Klass, typename BaseKlass, typename Ret, typename... Args>
	struct MemberWrapper<Ret (BaseKlass::*)(Args...), Klass>:
		MethodWrapperImpl<Ret (BaseKlass::*)(Args...), Klass>::Implementation {};

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

	// Function wrapper implementation for functions with the return type Ret and the parmeter types
	// Args...
	template <typename Ret, typename... Args>
	struct FunctionWrapperImpl {
		// Implements 'invoke' for function with a return value.
		template <size_t... Indices>
		struct ImplementationNonVoid {
			template <Ret (* func)(Args...)> static inline
			int invoke(State* state) {
				return static_cast<int>(
					pushReturn(
						state,
						func(
							// Read parameters off the stack and pass them to the function.
							read<Args>(state, 1 + Indices)...
						)
					)
				);
			}
		};

		// Implement 'invoke' for functions without a return value.
		template <size_t... Indices>
		struct ImplementationVoid {
			template <void (* func)(Args...)> static inline
			int invoke(State* state) {
				func(
					// Read parameters off the stack and pass them to the function.
					read<Args>(state, 1 + Indices)...
				);

				return 0;
			}
		};

		template <size_t... Indices>
		using ImplementationPicker =
			// Choose which implementation to use based on the return type of the function.
			typename std::conditional<
				std::is_same<Ret, void>::value,
				ImplementationVoid<Indices...>,
				ImplementationNonVoid<Indices...>
			>::type;

		using Implementation =
			// Generate an index sequence which is used by the selected implementation in order to
			// efficiently retrieve the parameters from the stack.
			typename MakeIndexSequence<sizeof...(Args)>::template Relay<
				ImplementationPicker
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
///
/// \param entity Qualified named of the entity that shall be wrapped
/// \returns Wrapped entity as `lua_CFunction`
#define LUWRA_WRAP(entity) \
	(&luwra::internal::Wrapper<decltype(&entity)>::template invoke<&entity>)

/// Same as `LUWRA_WRAP` but specifically for members of a class. It is imperative that you use this
/// macro instead of `LUWRA_WRAP` when working with inherited members.
///
/// \param base Qualified name of the targeted class
/// \param name Unqualified name of the member that shall be wrapped
/// \returns Wrapped entity as `lua_CFunction`
#define LUWRA_WRAP_MEMBER(base, name) \
	(&luwra::internal::MemberWrapper<decltype(&__LUWRA_NS_RESOLVE(base, name)), base>::template invoke<&__LUWRA_NS_RESOLVE(base, name)>)

#endif

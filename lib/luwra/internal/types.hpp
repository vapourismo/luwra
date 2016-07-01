/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_INTERNAL_TYPES_H_
#define LUWRA_INTERNAL_TYPES_H_

#include "common.hpp"
#include "typelist.hpp"

LUWRA_INTERNAL_NS_BEGIN

// Do something with the provided type.
template <typename Type>
struct With {
	// Construct a signature.
	template <typename... Args>
	using ConstructSignature = Type (Args...);
};

// Function object
template <typename Klass>
struct CallableInfo:
	CallableInfo<decltype(&Klass::operator ())> {};

// Function signature
template <typename Ret, typename... Args>
struct CallableInfo<Ret (Args...)> {
	// A call to an instance would evaluate to this type
	using ReturnType = Ret;

	// Parameter type list
	using Arguments = TypeList<Args...>;

	// Pass the template parameter pack to another template
	template <template <typename...> class Target>
	using RelayArguments = Target<Args...>;

	// Callable signature
	using Signature = Ret (Args...);
};

// Function pointer
template <typename Ret, typename... Args>
struct CallableInfo<Ret (*)(Args...)>:
	CallableInfo<Ret (Args...)> {};

// Method pointer
template <typename Klass, typename Ret, typename... Args>
struct CallableInfo<Ret (Klass::*)(Args...)>:
	CallableInfo<Ret (Args...)> {};

// Method pointer on const-qualified this
template <typename Klass, typename Ret, typename... Args>
struct CallableInfo<Ret (Klass::*)(Args...) const>:
	CallableInfo<Ret (Args...)> {};

// Method pointer on volatile-qualified this
template <typename Klass, typename Ret, typename... Args>
struct CallableInfo<Ret (Klass::*)(Args...) volatile>:
	CallableInfo<Ret (Args...)> {};

// Method pointer on const-volatile-qualified this
template <typename Klass, typename Ret, typename... Args>
struct CallableInfo<Ret (Klass::*)(Args...) const volatile>:
	CallableInfo<Ret (Args...)> {};

// Remove const qualification
template <typename Callable>
struct CallableInfo<const Callable>:
	CallableInfo<Callable> {};

// Remove volatile qualification
template <typename Callable>
struct CallableInfo<volatile Callable>:
	CallableInfo<Callable> {};

// Remove const-volatile qualification
template <typename Callable>
struct CallableInfo<const volatile Callable>:
	CallableInfo<Callable> {};

// Remove lvalue reference
template <typename Callable>
struct CallableInfo<Callable&>:
	CallableInfo<Callable> {};

// Remove rvalue reference
template <typename Callable>
struct CallableInfo<Callable&&>:
	CallableInfo<Callable> {};

template <typename Callable>
using ReturnTypeOf = typename CallableInfo<Callable>::ReturnType;

template <typename Callable>
using ArgumentsOf = typename CallableInfo<Callable>::Arguments;

template <typename Callable>
using SignatureOf = typename CallableInfo<Callable>::Signature;

// Catch usage error.
template <typename T>
struct MemberInfo {
	static_assert(
		sizeof(T) == -1,
		"Template parameter to MemberInfo is not a member type"
	);
};

// Method pointer
template <typename Klass, typename Ret, typename... Args>
struct MemberInfo<Ret (Klass::*)(Args...)>:
	CallableInfo<Ret (Klass::*)(Args...)>
{
	using MemberOf = Klass;
};

// Method pointer on const-qualified this
template <typename Klass, typename Ret, typename... Args>
struct MemberInfo<Ret (Klass::*)(Args...) const>:
	MemberInfo<Ret (Klass::*)(Args...)> {};

// Method pointer on volatile-qualified this
template <typename Klass, typename Ret, typename... Args>
struct MemberInfo<Ret (Klass::*)(Args...) volatile>:
	MemberInfo<Ret (Klass::*)(Args...)> {};

// Method pointer on const-volatile-qualified this
template <typename Klass, typename Ret, typename... Args>
struct MemberInfo<Ret (Klass::*)(Args...) const volatile>:
	MemberInfo<Ret (Klass::*)(Args...)> {};

// Field accessor
template <typename Klass, typename Field>
struct MemberInfo<Field Klass::*> {
	using MemberOf = Klass;
	using FieldType = Field;
};

// Remove const qualification
template <typename Member>
struct MemberInfo<const Member>:
	MemberInfo<Member> {};

// Remove volatile qualification
template <typename Member>
struct MemberInfo<volatile Member>:
	MemberInfo<Member> {};

// Remove const-volatile qualification
template <typename Member>
struct MemberInfo<const volatile Member>:
	MemberInfo<Member> {};

// Remove lvalue reference
template <typename Member>
struct MemberInfo<Member&>:
	MemberInfo<Member> {};

// Remove rvalue reference
template <typename Member>
struct MemberInfo<Member&&>:
	MemberInfo<Member> {};

LUWRA_INTERNAL_NS_END

#endif

#ifndef LUWR_INTERNAL_CALLABLEINFO_H_
#define LUWR_INTERNAL_CALLABLEINFO_H_

#include "common.hpp"
#include "typelist.hpp"

LUWRA_INTERNAL_NS_BEGIN

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

LUWRA_INTERNAL_NS_END

#endif

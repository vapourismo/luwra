/* Luwra
 * Minimal-overhead Lua wrapper for C++
 *
 * Copyright (C) 2016, Ole Krüger <ole@vprsm.de>
 */

#ifndef LUWRA_INTERNAL_H_
#define LUWRA_INTERNAL_H_

#include "common.hpp"
#include "compat.hpp"

LUWRA_NS_BEGIN

namespace internal {
	// Container for a list of types
	template <typename... Types>
	struct TypeList {
		// Add types to the type list
		template <typename... OtherTypes>
		using AddTypes = TypeList<Types..., OtherTypes...>;

		// Pass type list to a template
		template <template <typename...> class Receiver>
		using RelayTypes = Receiver<Types...>;

		// Construct a signature
		template <typename Ret>
		using ConstructSignature = Ret (Types...);
	};

	// Concat two TypeLists
	template <typename Left, typename Right>
	using ConcatTypeList =
		typename Right::template RelayTypes<Left::template AddTypes>;

	template <typename Seq, typename List>
	struct _DropFromTypeList {
		static_assert(
			sizeof(List) == -1,
			"Template parameters to _DropFromTypeList are invalid"
		);
	};

	template <typename... Types>
	struct _DropFromTypeList<IndexSequence<>, TypeList<Types...>> {
		using Result = TypeList<Types...>;
	};

	template <size_t Index, size_t... IndexTail, typename Head, typename... Tail>
	struct _DropFromTypeList<IndexSequence<Index, IndexTail...>, TypeList<Head, Tail...>>:
		_DropFromTypeList<IndexSequence<IndexTail...>, TypeList<Tail...>> {};

	template <size_t N, typename List>
	using DropFromTypeList = typename _DropFromTypeList<MakeIndexSequence<N>, List>::Result;

	// Information about function objects
	template <typename Klass>
	struct CallableInfo:
		CallableInfo<decltype(&Klass::operator ())> {};

	// Information about function signature
	template <typename Ret, typename... Args>
	struct CallableInfo<Ret (Args...)> {
		// A call to an instance would evaluate to this type
		using ReturnType = Ret;

		// Parameter type list
		using ArgumentTypeList = TypeList<Args...>;

		// Pass the template parameter pack to another template
		template <template <typename...> class Target>
		using RelayArguments = Target<Args...>;

		// Callable signature
		using Signature = Ret (Args...);

		// Pass the signature of this callable to another template
		template <template <typename> class Target>
		using RelaySignature = Target<Signature>;
	};

	// Information about function pointers
	template <typename Ret, typename... Args>
	struct CallableInfo<Ret (*)(Args...)>:
		CallableInfo<Ret (Args...)> {};

	// Information about method pointers
	template <typename Klass, typename Ret, typename... Args>
	struct CallableInfo<Ret (Klass::*)(Args...)>:
		CallableInfo<Ret (Args...)> {};

	template <typename Klass, typename Ret, typename... Args>
	struct CallableInfo<Ret (Klass::*)(Args...) const>:
		CallableInfo<Ret (Args...)> {};

	template <typename Klass, typename Ret, typename... Args>
	struct CallableInfo<Ret (Klass::*)(Args...) volatile>:
		CallableInfo<Ret (Args...)> {};

	template <typename Klass, typename Ret, typename... Args>
	struct CallableInfo<Ret (Klass::*)(Args...) const volatile>:
		CallableInfo<Ret (Args...)> {};

	// Useful aliases
	template <typename Callable>
	struct CallableInfo<const Callable>:
		CallableInfo<Callable> {};

	template <typename Callable>
	struct CallableInfo<volatile Callable>:
		CallableInfo<Callable> {};

	template <typename Callable>
	struct CallableInfo<Callable&>:
		CallableInfo<Callable> {};

	template <typename Callable>
	struct CallableInfo<Callable&&>:
		CallableInfo<Callable> {};

	template <typename T>
	struct MemberInfo {
		static_assert(sizeof(T) == -1, "Template parameter to MemberInfo is not a member type");
	};

	// Information about methods
	template <typename Klass, typename Ret, typename... Args>
	struct MemberInfo<Ret (Klass::*)(Args...)>:
		CallableInfo<Ret (Klass::*)(Args...)>
	{
		using MemberOf = Klass;
	};

	template <typename Klass, typename Ret, typename... Args>
	struct MemberInfo<Ret (Klass::*)(Args...) const>:
		MemberInfo<Ret (Klass::*)(Args...)> {};

	template <typename Klass, typename Ret, typename... Args>
	struct MemberInfo<Ret (Klass::*)(Args...) volatile>:
		MemberInfo<Ret (Klass::*)(Args...)> {};

	template <typename Klass, typename Ret, typename... Args>
	struct MemberInfo<Ret (Klass::*)(Args...) const volatile>:
		MemberInfo<Ret (Klass::*)(Args...)> {};

	// Information about fields
	template <typename Klass, typename Field>
	struct MemberInfo<Field Klass::*> {
		using MemberOf = Klass;
		using FieldType = Field;
	};

	// Check whether
	template <
		template <typename, typename> class,
		typename,
		typename
	>
	struct IsPrefixOf:
		BoolConstant<false> {};

	template <
		template <typename, typename> class Compare,
		typename... Rest
	>
	struct IsPrefixOf<Compare, TypeList<>, TypeList<Rest...>>:
		BoolConstant<true> {};

	template <
		template <typename, typename> class Compare,
		typename Prefix,
		typename Base,
		typename... BaseTail
	>
	struct IsPrefixOf<Compare, TypeList<Prefix>, TypeList<Base, BaseTail...>>:
		Compare<Prefix, Base> {};

	template <
		template <typename, typename> class Compare,
		typename Prefix,
		typename... PrefixTail,
		typename Base,
		typename... BaseTail
	>
	struct IsPrefixOf<Compare, TypeList<Prefix, PrefixTail...>, TypeList<Base, BaseTail...>>:
		BoolConstant<
			Compare<Prefix, Base>::value &&
			IsPrefixOf<Compare, TypeList<PrefixTail...>, TypeList<BaseTail...>>::value
		> {};

	template <
		template <typename, typename> class,
		typename,
		typename
	>
	struct Match:
		BoolConstant<false> {};

	template <
		template <typename, typename> class Compare
	>
	struct Match<Compare, TypeList<>, TypeList<>>:
		BoolConstant<true> {};

	template <
		template <typename, typename> class Compare,
		typename Left,
		typename Right
	>
	struct Match<Compare, TypeList<Left>, TypeList<Right>>:
		Compare<Left, Right> {};

	template <
		template <typename, typename> class Compare,
		typename Left,
		typename... LeftTail,
		typename Right,
		typename... RightTail
	>
	struct Match<Compare, TypeList<Left, LeftTail...>, TypeList<Right, RightTail...>>:
		BoolConstant<
			Compare<Left, Right>::value &&
			Match<Compare, TypeList<LeftTail...>, TypeList<RightTail...>>::value
		> {};
}

LUWRA_NS_END

#endif

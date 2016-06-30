#include <catch.hpp>
#include <luwra/internal/types.hpp>
#include <type_traits>

using namespace luwra::internal;

TEST_CASE("With") {
	using Subject = With<char>;

	SECTION("ConstructSignature") {
		using Result1 = Subject::ConstructSignature<>;
		using Expected1 = char ();

		REQUIRE((std::is_same<Result1, Expected1>::value));

		using Result2 = Subject::ConstructSignature<short, int>;
		using Expected2 = char (short, int);

		REQUIRE((std::is_same<Result2, Expected2>::value));
	}
}

namespace {
	struct A {
		char operator ()();
	};

	struct B {
		short operator ()(char);
	};

	struct C {
		int operator ()(short, char);
	};
}

TEST_CASE("CallableInfo") {
	using Subject1 = char ();
	using Subject2 = short (char);
	using Subject3 = int (short, char);

	using Subject4 = char (A::*)();
	using Subject5 = short (B::*)(char);
	using Subject6 = int (C::*)(short, char);

	using Subject7 = A;
	using Subject8 = B;
	using Subject9 = C;

	SECTION("ReturnType") {
		using Result1 = CallableInfo<Subject1>::ReturnType;
		using Expected1 = char;

		REQUIRE((std::is_same<Result1, Expected1>::value));

		using Result2 = CallableInfo<Subject2>::ReturnType;
		using Expected2 = short;

		REQUIRE((std::is_same<Result2, Expected2>::value));

		using Result3 = CallableInfo<Subject3>::ReturnType;
		using Expected3 = int;

		REQUIRE((std::is_same<Result3, Expected3>::value));

		using Result4 = CallableInfo<Subject4>::ReturnType;
		using Expected4 = char;

		REQUIRE((std::is_same<Result4, Expected4>::value));

		using Result5 = CallableInfo<Subject5>::ReturnType;
		using Expected5 = short;

		REQUIRE((std::is_same<Result5, Expected5>::value));

		using Result6 = CallableInfo<Subject6>::ReturnType;
		using Expected6 = int;

		REQUIRE((std::is_same<Result6, Expected6>::value));

		using Result7 = CallableInfo<Subject7>::ReturnType;
		using Expected7 = char;

		REQUIRE((std::is_same<Result7, Expected7>::value));

		using Result8 = CallableInfo<Subject8>::ReturnType;
		using Expected8 = short;

		REQUIRE((std::is_same<Result8, Expected8>::value));

		using Result9 = CallableInfo<Subject9>::ReturnType;
		using Expected9 = int;

		REQUIRE((std::is_same<Result9, Expected9>::value));
	}

	SECTION("Arguments") {
		using Result1 = CallableInfo<Subject1>::Arguments;
		using Expected1 = TypeList<>;

		REQUIRE((std::is_same<Result1, Expected1>::value));

		using Result2 = CallableInfo<Subject2>::Arguments;
		using Expected2 = TypeList<char>;

		REQUIRE((std::is_same<Result2, Expected2>::value));

		using Result3 = CallableInfo<Subject3>::Arguments;
		using Expected3 = TypeList<short, char>;

		REQUIRE((std::is_same<Result3, Expected3>::value));

		using Result4 = CallableInfo<Subject4>::Arguments;
		using Expected4 = TypeList<>;

		REQUIRE((std::is_same<Result4, Expected4>::value));

		using Result5 = CallableInfo<Subject5>::Arguments;
		using Expected5 = TypeList<char>;

		REQUIRE((std::is_same<Result5, Expected5>::value));

		using Result6 = CallableInfo<Subject6>::Arguments;
		using Expected6 = TypeList<short, char>;

		REQUIRE((std::is_same<Result6, Expected6>::value));

		using Result7 = CallableInfo<Subject7>::Arguments;
		using Expected7 = TypeList<>;

		REQUIRE((std::is_same<Result7, Expected7>::value));

		using Result8 = CallableInfo<Subject8>::Arguments;
		using Expected8 = TypeList<char>;

		REQUIRE((std::is_same<Result8, Expected8>::value));

		using Result9 = CallableInfo<Subject9>::Arguments;
		using Expected9 = TypeList<short, char>;

		REQUIRE((std::is_same<Result9, Expected9>::value));
	}

	SECTION("Signature") {
		using Result1 = CallableInfo<Subject1>::Signature;
		using Expected1 = char ();

		REQUIRE((std::is_same<Result1, Expected1>::value));

		using Result2 = CallableInfo<Subject2>::Signature;
		using Expected2 = short (char);

		REQUIRE((std::is_same<Result2, Expected2>::value));

		using Result3 = CallableInfo<Subject3>::Signature;
		using Expected3 = int (short, char);

		REQUIRE((std::is_same<Result3, Expected3>::value));

		using Result4 = CallableInfo<Subject4>::Signature;
		using Expected4 = char ();

		REQUIRE((std::is_same<Result4, Expected4>::value));

		using Result5 = CallableInfo<Subject5>::Signature;
		using Expected5 = short (char);

		REQUIRE((std::is_same<Result5, Expected5>::value));

		using Result6 = CallableInfo<Subject6>::Signature;
		using Expected6 = int (short, char);

		REQUIRE((std::is_same<Result6, Expected6>::value));

		using Result7 = CallableInfo<Subject7>::Signature;
		using Expected7 = char ();

		REQUIRE((std::is_same<Result7, Expected7>::value));

		using Result8 = CallableInfo<Subject8>::Signature;
		using Expected8 = short (char);

		REQUIRE((std::is_same<Result8, Expected8>::value));

		using Result9 = CallableInfo<Subject9>::Signature;
		using Expected9 = int (short, char);

		REQUIRE((std::is_same<Result9, Expected9>::value));
	}
}

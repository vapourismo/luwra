#include <catch.hpp>
#include <luwra/internal/typelist.hpp>
#include <type_traits>

using namespace luwra::internal;

TEST_CASE("TypeList") {
	SECTION("Add") {
		using Subject = TypeList<>;

		using Result1 = Subject::Add<short>;
		using Expected1 = TypeList<short>;

		REQUIRE((std::is_same<Result1, Expected1>::value));

		using Result2 = Result1::Add<int>;
		using Expected2 = TypeList<short, int>;

		REQUIRE((std::is_same<Result2, Expected2>::value));
	}

	SECTION("Relay") {
		using Subject = TypeList<char, short>;

		using Result = Subject::Relay<TypeList>;
		using Expected = Subject;

		REQUIRE((std::is_same<Result, Expected>::value));
	}

	SECTION("Drop") {
		using Subject1 = TypeList<>;

		using Result1 = Subject1::Drop<1>;
		using Expected1 = TypeList<>;

		REQUIRE((std::is_same<Result1, Expected1>::value));

		using Subject2 = TypeList<char, short, int>;

		using Result2 = Subject2::Drop<2>;
		using Expected2 = TypeList<int>;

		REQUIRE((std::is_same<Result2, Expected2>::value));

		using Result3 = Result2::Drop<1>;
		using Expected3 = TypeList<>;

		REQUIRE((std::is_same<Result3, Expected3>::value));
	}

	SECTION("Append") {
		using Subject1 = TypeList<char, short>;
		using Subject2 = TypeList<int, long>;

		using Result1 = Subject1::Append<Subject2>;
		using Expected1 = TypeList<char, short, int, long>;

		REQUIRE((std::is_same<Result1, Expected1>::value));

		using Result2 = Subject2::Append<Subject1>;
		using Expected2 = TypeList<int, long, char, short>;

		REQUIRE((std::is_same<Result2, Expected2>::value));
	}
}

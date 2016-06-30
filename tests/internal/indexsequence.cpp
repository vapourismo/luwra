#include <catch.hpp>
#include <luwra/internal/indexsequence.hpp>
#include <type_traits>

using namespace luwra::internal;

TEST_CASE("MakeIndexSequence") {
	using Result1 = MakeIndexSequence<0>;
	using Expected1 = IndexSequence<>;

	REQUIRE((std::is_same<Result1, Expected1>::value));

	using Result2 = MakeIndexSequence<10>;
	using Expected2 = IndexSequence<0, 1, 2, 3, 4, 5, 6, 7, 8, 9>;

	REQUIRE((std::is_same<Result2, Expected2>::value));
}

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

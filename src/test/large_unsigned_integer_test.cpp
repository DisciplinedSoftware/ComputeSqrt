#include "../large_unsigned_integer.hpp"

#include <numeric>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("large_unsigned_integer") {
    using namespace std::string_literals;

    SECTION("Construction") {
        CHECK(large_unsigned_integer() == 0UL);
        CHECK(large_unsigned_integer(0u) == 0UL);
        CHECK(large_unsigned_integer::from_string("0"s).value() == 0UL);
    }

    SECTION("Equality operator") {
        CHECK(large_unsigned_integer(1u) == 1u);
        CHECK(large_unsigned_integer(123456789012UL) == 123456789012UL);
        CHECK(large_unsigned_integer(-123456789012UL) == -123456789012UL);
    }

    SECTION("Smaller than operator") {
        CHECK((large_unsigned_integer(123456789011UL) < large_unsigned_integer(1UL)) == false);
        CHECK((large_unsigned_integer(1UL) < large_unsigned_integer(123456789012UL)) == true);
        CHECK((large_unsigned_integer(123456789012UL) < large_unsigned_integer(123456789011UL)) == false);
        CHECK((large_unsigned_integer(123456789012UL) < large_unsigned_integer(123456789012UL)) == false);
        CHECK((large_unsigned_integer(123456789011UL) < large_unsigned_integer(123456789012UL)) == true);
    }

    SECTION("Addition") {
        CHECK(large_unsigned_integer(123456789012UL) + large_unsigned_integer(123456789012UL) == 246913578024UL);
        CHECK(large_unsigned_integer::from_string("12345678901234567890").value() + large_unsigned_integer::from_string("9876543211234567890").value() == large_unsigned_integer::from_string("22222222112469135780").value());
    }

    SECTION("Subtraction") {
        CHECK(large_unsigned_integer(123456789012UL) - large_unsigned_integer(123456789012UL) == 0UL);
        CHECK(large_unsigned_integer(123456789012UL) - large_unsigned_integer(123456789000UL) == 12UL);

        CHECK(large_unsigned_integer::from_string("42010168383160134110440665745547766649977556245").value() - large_unsigned_integer::from_string("42010168383160134110440665745547766649977556200").value() == 45u);
        // Large number with multiple carries
        CHECK(large_unsigned_integer::from_string("6779575297923493247898029418281817537676227380624747815049013732411535947860165631075856579568246233910811591607874120563664388642279371457390259857568960958935772908009048011104104746617436179252684469483776429833549880669503680760948677500"s).value() - large_unsigned_integer::from_string("6480740698407860230965967436087996657705204307058346549711354397809617377844044371400360906605610235675450542097411694335491913404906608688945818961664673951305585227822636095668822680668761521776633672599142812990432160139844957280499363525"s).value() == large_unsigned_integer::from_string("298834599515633016932061982193820879971023073566401265337659334601918570016121259675495672962635998235361049510462426228172475237372762768444440895904287007630187680186411915435282065948674657476050796884633616843117720529658723480449313975").value());
    }

    SECTION("Multiplication") {
        CHECK(large_unsigned_integer(246913578024UL) * large_unsigned_integer(123456789012UL) == large_unsigned_integer::from_string("30483157506306967872288"s).value());
        CHECK(large_unsigned_integer::from_string("42010168383160134110440665745547766649977556245"s).value() * large_unsigned_integer::from_string("1234567890987654321").value() == large_unsigned_integer::from_string("51864404980834242630409449768792397904982098404496001028394784645").value());
    }
}
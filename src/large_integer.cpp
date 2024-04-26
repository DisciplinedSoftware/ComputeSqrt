#include "large_integer.h"

#include <catch2/catch_test_macros.hpp>

std::istream &operator>>(std::istream &stream_, large_integer &value_)
{
    // Assume that the rdbuf exist and that the number is fully contains in the
    // buffer
    assert(stream_.rdbuf() != nullptr);
    const std::streamsize size = stream_.rdbuf()->in_avail();
    std::string number(size, 0);
    stream_.read(number.data(), size);

    auto result = large_integer::from_string(number);
    assert(result.has_value());
    value_ = std::move(result).value_or(0);

    return stream_;
}

std::ostream &operator<<(std::ostream &stream_, const large_integer &value_)
{
    stream_ << to_string(value_);
    return stream_;
}

TEST_CASE("large_integer")
{
    using namespace std::string_literals;
    CHECK(large_integer(1) == 1);
    CHECK(large_integer(-1) == -1);
    CHECK(large_integer(123456789012L) == 123456789012L);
    CHECK(large_integer(-123456789012L) == -123456789012L);

    CHECK((large_integer(123456789011L) < large_integer(1L)) == false);
    CHECK((large_integer(1L) < large_integer(123456789012L)) == true);
    CHECK((large_integer(123456789012L) < large_integer(123456789011L)) == false);
    CHECK((large_integer(123456789012L) < large_integer(123456789012L)) == false);
    CHECK((large_integer(123456789011L) < large_integer(123456789012L)) == true);

    CHECK(large_integer(123456789012L) + large_integer(123456789012L) == 246913578024L);
    CHECK(large_integer(-123456789012L) + large_integer(-123456789012L) == -246913578024L);
    CHECK(large_integer(123456789012L) + large_integer(-123456789012L) == 0);
    CHECK(large_integer(-123456789012L) + large_integer(123456789012L) == 0);
    CHECK(large_integer(123456789012L) + large_integer(-123456789000L) == 12);
    CHECK(large_integer(246913578024L) + large_integer(-123456789012L) == 123456789012L);
    CHECK(large_integer(-246913578024L) + large_integer(123456789012L) == -123456789012L);
    CHECK(large_integer(-123456789012L) + large_integer(246913578024L) == 123456789012L);
    CHECK(large_integer(123456789012L) + large_integer(-246913578024L) == -123456789012L);

    CHECK(large_integer::from_string("42010168383160134110440665745547766649977556245").value() == large_integer::from_string("42010168383160134110440665745547766649977556245"s).value());
    CHECK(large_integer::from_string("-42010168383160134110440665745547766649977556245").value() == large_integer::from_string("-42010168383160134110440665745547766649977556245"s).value());

    CHECK(large_integer::from_string("42010168383160134110440665745547766649977556245").value() - large_integer::from_string("42010168383160134110440665745547766649977556200").value() == 45);

    CHECK(large_integer(246913578024L) * large_integer(123456789012L) == large_integer::from_string("30483157506306967872288"s).value());
    CHECK(large_integer(-246913578024L) * large_integer(-123456789012L) == large_integer::from_string("30483157506306967872288"s).value());
    CHECK(large_integer(246913578024L) * large_integer(-123456789012L) == large_integer::from_string("-30483157506306967872288"s).value());
    CHECK(large_integer(-246913578024L) * large_integer(123456789012L) == large_integer::from_string("-30483157506306967872288"s).value());
    CHECK(large_integer(-123456789012L) * large_integer(246913578024L) == large_integer::from_string("-30483157506306967872288"s).value());
    CHECK(large_integer(123456789012L) * large_integer(-246913578024L) == large_integer::from_string("-30483157506306967872288"s).value());

    CHECK(large_integer::from_string("42010168383160134110440665745547766649977556245"s).value() * large_integer::from_string("1234567890987654321").value() == large_integer::from_string("51864404980834242630409449768792397904982098404496001028394784645").value());

    CHECK(large_integer::from_string("6779575297923493247898029418281817537676227380624747815049013732411535947860165631075856579568246233910811591607874120563664388642279371457390259857568960958935772908009048011104104746617436179252684469483776429833549880669503680760948677500"s).value() - large_integer::from_string("6480740698407860230965967436087996657705204307058346549711354397809617377844044371400360906605610235675450542097411694335491913404906608688945818961664673951305585227822636095668822680668761521776633672599142812990432160139844957280499363525"s).value() == large_integer::from_string("298834599515633016932061982193820879971023073566401265337659334601918570016121259675495672962635998235361049510462426228172475237372762768444440895904287007630187680186411915435282065948674657476050796884633616843117720529658723480449313975").value());
}
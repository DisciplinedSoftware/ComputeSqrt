#ifndef SQUARE_ROOT_HPP
#define SQUARE_ROOT_HPP

#include <algorithm>
#include <cassert>
#include <cmath>
#include <concepts>
#include <numeric>
#include <optional>
#include <ranges>
#include <string>
#include <stop_token>
#include <tuple>
#include <utility>
#include <vector>

#include "generator.hpp"
#include "large_unsigned_integer.hpp"
#include "utility.hpp"

// ----------------------------------------------------------------------------

namespace details {

// ----------------------------------------------------------------------------
// Helper class to compute digits one at a time
class square_root_next_digit_computer {
public:
    [[nodiscard]] unsigned int operator()(auto current_);
    [[nodiscard]] bool has_next_digit() const;

private:
    large_unsigned_integer remainder{ 0u };
    large_unsigned_integer result{ 0u };
};

[[nodiscard]] std::vector<unsigned int> split_integer_into_groups_of_2_digits(std::integral auto value_) {
    assert(value_ != NAN && value_ >= 0);

    std::vector<unsigned int> integer_values;

    auto residual = value_;
    while (residual > 0) {
        integer_values.emplace_back(static_cast<unsigned int>(residual % 100));
        residual /= 100;
    }

    return integer_values;
}

generator<unsigned int> compute_integral_part_of_square_root(std::integral auto value_, square_root_next_digit_computer& computer_) {
    const auto integer_values = split_integer_into_groups_of_2_digits(value_);

    const auto inputs
        = std::views::reverse(integer_values)
        | std::views::transform(std::ref(computer_));

    for (auto x : inputs) {
        co_yield x;
    }
}

// ----------------------------------------------------------------------------

generator<unsigned int> compute_fractional_part_of_square_root(square_root_next_digit_computer& computer_);

}

// ----------------------------------------------------------------------------

generator<char> compute_square_root_digit_by_digit_method(std::integral auto value_) {
    assert(value_ != NAN && value_ >= 0);

    // Early return optimization
    if (value_ == 0 || value_ == 1) {
        co_yield to_char(value_);
        co_return;
    }

    details::square_root_next_digit_computer computer;

    auto integral_generator = details::compute_integral_part_of_square_root(value_, computer);
    while (integral_generator.has_value()) {
        co_yield to_char(integral_generator.value());
    }

    // Early return optimization when the number is a perfect square
    if (!computer.has_next_digit()) {
        co_return;
    }

    co_yield '.';

    auto fractional_generator = details::compute_fractional_part_of_square_root(computer);

    while (fractional_generator.has_value()) {
        co_yield to_char(fractional_generator.value());
    }
}

// ----------------------------------------------------------------------------
// Stream the value of the square root one (decimal) digit at a time
void compute_square_root_digit_by_digit_method(std::ostream& stream_, std::integral auto value_, std::stop_token stop_) {
    // NaN is a special case
    if (value_ == NAN) { // std::isfinite with integer is not mandatory in the standard
        stream_ << NAN;
        return;
    }

    // Cannot calculate the root of a negative number
    if (value_ < 0) {
        stream_ << NAN;
        return;
    }

    auto generator = compute_square_root_digit_by_digit_method(value_);
    while (!stop_.stop_requested() && generator.has_value()) {
        stream_ << generator.value() << std::flush; // Flush stream everytime for smoother display
    }
}

#endif // SQUARE_ROOT_HPP
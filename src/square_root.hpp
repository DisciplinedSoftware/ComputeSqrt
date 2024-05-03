#ifndef SQUARE_ROOT_HPP
#define SQUARE_ROOT_HPP

#include <algorithm>
#include <cassert>
#include <cmath>
#include <concepts>
#include <numbers>
#include <numeric>
#include <optional>
#include <ranges>
#include <string>
#include <stop_token>
#include <tuple>
#include <utility>
#include <vector>

#include "large_integer.hpp"

// ----------------------------------------------------------------------------
// Arbitrarily long definition of sqrt(42)
#define SQRT42 6.4807406984078602309659674360879966577052043070583465497113543978

// ----------------------------------------------------------------------------
// Constant expression of sqrt(42)
template <typename T> inline constexpr T sqrt42 = static_cast<T>(SQRT42);

namespace details {

template <std::floating_point T>
[[nodiscard]] constexpr auto compute_square_root_exception(T value_, std::invocable<T> auto func_) -> T {
#if __cpp_lib_constexpr_cmath >= 202202L
    if (!std::isfinite(value_)) {
#else
    if (value_ == NAN || value_ == std::numeric_limits<T>::infinity() || value_ == -std::numeric_limits<T>::infinity()) {
#endif
        return value_;
    }

    // Cannot calculate the root of a negative number
    if (value_ < 0) {
        return NAN;
    }

    // Early return for 0 or 1
    if (value_ == T{ 0 } || value_ == T{ 1 }) {
        return value_;
    }

    return func_(value_);
}

// ----------------------------------------------------------------------------
// Split value into its fractional and exponent part with the exponent always
// even fractional part is (-2, -0.25], [0.25, 2) return [fractional, exponent]
template <typename T>
[[nodiscard]]
#if __cpp_lib_constexpr_cmath >= 202202L
constexpr
#endif
std::pair<T, int> split_into_fractional_and_even_exponent(T value_) {
    int exponent{};
    T fractional = std::frexp(value_, &exponent);

    // Check is exponent is odd by looking at the first bit
    if (exponent & 1) {
        if (exponent < 0) {
            fractional /= 2;
            ++exponent;
        } else {
            fractional *= 2;
            --exponent;
        }
    }

    return { fractional, exponent };
}

// ----------------------------------------------------------------------------
// Compute square root using the factional and exponent optimization
// Split the problem into the fractional and the exponent parts since:
// sqrt(frac*2^exp)
// sqrt(frac)*sqrt(2^exp)
// sqrt(frac)*2^(exp/2)
// To make this optimization works the exponent must be even
// This is less optimal when the value is a perfect square
template <std::floating_point T>
[[nodiscard]]
#if __cpp_lib_constexpr_cmath >= 202202L
constexpr
#endif
auto compute_square_root_using_fractional_and_exponent_optimization(
    T value_, std::invocable<T> auto func_) -> T {
    // Split the problem into the fractional and the exponent parts
    const auto [fractional, exponent] =
        details::split_into_fractional_and_even_exponent(value_);

    // Early return for 0 or 1
    if (fractional == T{ 0 } || fractional == T{ 1 }) {
        return std::ldexp(fractional, exponent / 2);
    }

    const auto result = func_(fractional);

    // Reconstruct the result
    return std::ldexp(result, exponent / 2);
}

// ----------------------------------------------------------------------------
// Compute square root of the fractional part
template <std::floating_point T>
[[nodiscard]] auto compute_square_root_binary_search_method_fractional(T fractional_) -> T {
    assert(0 <= fractional_ && fractional_ < 2);

    // Search from zero to fractional_
    // If fractional_ is less than 1, then the result will be greater than
    // fractional_, hence the search is from 0 to fractional_ + 1 Since
    // fractional_ is between [0.25,2) the sqrt will be between 0 and, at worst,
    // sqrt(2), hence the maximum is fixed to sqrt(2)
    T left = 0;
    T right = fractional_ >= 1
        ? std::min(fractional_, std::numbers::sqrt2_v<T>)
        : std::min(fractional_ + 1, std::numbers::sqrt2_v<T>);
    T old = std::numeric_limits<T>::max();

    while (true) {
        // Compute the middle point
        const T middle = std::midpoint(left, right);

        // No more convergence
        if (middle == old) {
            return middle;
        }

        const T square = middle * middle;

        // Early return optimization as the value was found
        if (square == fractional_) {
            return middle;
        }

        // Reduce the interval
        if (square < fractional_) {
            left = middle;
        } else {
            right = middle;
        }

        old = middle;
    }
}

} // namespace details

// ----------------------------------------------------------------------------
// Compute the square root using a binary search
template <std::floating_point T>
[[nodiscard]] auto compute_square_root_binary_search_method(T value_) -> T {
    return details::compute_square_root_exception(value_, [](T value_) {
        return details::compute_square_root_using_fractional_and_exponent_optimization(
            value_,
            details::compute_square_root_binary_search_method_fractional<T>);
    });
}

// ----------------------------------------------------------------------------
// Overload for integral value
[[nodiscard]] double compute_square_root_binary_search_method(std::integral auto value_) {
    return compute_square_root_binary_search_method<double>(value_);
}

namespace details {

// ----------------------------------------------------------------------------
// Compute square root of the fractional part
template <std::floating_point T>
[[nodiscard]] constexpr auto compute_square_root_heron_method_fractional(T fractional_) -> T {
    T x = fractional_ / 2;
    T old = x;

    while (true) {
        x = std::midpoint(x, fractional_ / x);

        if (x == old) {
            return x;
        }

        old = x;
    }
}

} // namespace details

// ----------------------------------------------------------------------------
// Compute the square root using heron's method
template <std::floating_point T>
[[nodiscard]] auto compute_square_root_heron_method(T value_) -> T {
    return details::compute_square_root_exception(value_, [](T value_) {
        return details::compute_square_root_using_fractional_and_exponent_optimization(
            value_,
            details::compute_square_root_heron_method_fractional<T>);
    });
}

// ----------------------------------------------------------------------------
// Overload for integral value
[[nodiscard]] double compute_square_root_heron_method(std::integral auto value_) {
    return compute_square_root_heron_method<double>(value_);
}

// ----------------------------------------------------------------------------
// Helper function to check if a function is constexpr
template<std::invocable F, int = (F{}(), 0) >
[[nodiscard]] constexpr bool is_constexpr(F) { return true; }
[[nodiscard]] constexpr bool is_constexpr(...) { return false; }

// ----------------------------------------------------------------------------
// Compute the square root using heron's method
template <std::floating_point T>
[[nodiscard]] constexpr auto compute_square_root_heron_method_constexpr(T value_) -> T {
    return details::compute_square_root_exception(value_, [](T value_) {
        if constexpr (is_constexpr([]() { std::ignore = details::compute_square_root_using_fractional_and_exponent_optimization(T{ 0 }, [](auto) { return 0; }); })) {
            return details::compute_square_root_using_fractional_and_exponent_optimization(
                value_,
                details::compute_square_root_heron_method_fractional<T>);
        } else {
            return details::compute_square_root_heron_method_fractional(value_);
        }
    });
}

// ----------------------------------------------------------------------------
// Overload for integral value
[[nodiscard]] constexpr double compute_square_root_heron_method_constexpr(std::integral auto value_) {
    return compute_square_root_heron_method_constexpr<double>(value_);
}

namespace details {

// ----------------------------------------------------------------------------
// Compute square root of the fractional part
template <std::floating_point T>
[[nodiscard]] auto compute_square_root_bakhshali_method_fractional(T fractional_) -> T {
    T x = fractional_ / 2;
    T old = std::numeric_limits<T>::max();

    while (true) {
        const T a = (fractional_ / x - x) / 2;
        const T d = a - (a * a) / (2 * (x + a));
        const T x_temp = x + d;

        const T abs_epsilon = x_temp * x_temp - fractional_;

        // Are we closer to our goal
        if (abs_epsilon >= old) {
            return x_temp;
        }

        old = abs_epsilon;
        x = x_temp;
    }
}

} // namespace details

// ----------------------------------------------------------------------------
// Compute the square root using bakhshali's method
template <std::floating_point T>
[[nodiscard]] auto compute_square_root_bakhshali_method(T value_) -> T {
    return details::compute_square_root_exception(value_, [](T value_) {
        return details::
            compute_square_root_using_fractional_and_exponent_optimization(
                value_,
                details::compute_square_root_bakhshali_method_fractional<T>);
    });
}

// ----------------------------------------------------------------------------
// Overload for integral value
[[nodiscard]] double compute_square_root_bakhshali_method(std::integral auto value_) {
    return compute_square_root_bakhshali_method<double>(value_);
}

// ----------------------------------------------------------------------------

namespace details {

class square_root_digits_generator {
public:
    [[nodiscard]] unsigned int operator()(auto current_);
    [[nodiscard]] bool has_next_digit() const;

private:
    large_integer remainder{ 0 };
    large_integer result{ 0 };
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

[[nodiscard]] std::string compute_integral_part_of_square_root(std::integral auto value_, square_root_digits_generator& generator_) {
    const auto integer_values = split_integer_into_groups_of_2_digits(value_);

    auto integral_string_rng
        = std::views::reverse(integer_values)
        | std::views::transform(std::ref(generator_))
        | std::views::transform([](auto x) {
        assert(x < 10);
        return static_cast<std::string::value_type>(x) + '0';
    });

    return { std::begin(integral_string_rng), std::end(integral_string_rng) };
}

[[nodiscard]] std::string compute_fractional_part_of_square_root(unsigned int precision_, square_root_digits_generator& generator_);
[[nodiscard]] std::tuple<std::string, std::string> round_last_digit(std::string&& integral_part_, std::string&& fractional_part_, unsigned int rounding_digit_);
[[nodiscard]] std::string trim_lower_zeros(std::string&& fractional_part_);

[[nodiscard]] std::string compute_square_root_digit_by_digit_method(std::integral auto value_, unsigned int precision_) {
    assert(value_ != NAN && value_ >= 0);

    // Early return optimization
    if (value_ == 0 || value_ == 1) {
        return std::to_string(value_);
    }

    square_root_digits_generator generator;

    auto integral_part = compute_integral_part_of_square_root(value_, generator);

    // Early return optimization when the number is a perfect square
    if (!generator.has_next_digit()) {
        return integral_part;
    }

    auto fractional_part = compute_fractional_part_of_square_root(precision_, generator);

    constexpr const unsigned int next_value = 0;
    std::tie(integral_part, fractional_part) = round_last_digit(std::move(integral_part), std::move(fractional_part), generator(next_value));

    fractional_part = trim_lower_zeros(std::move(fractional_part));

    if (fractional_part.empty()) {
        return integral_part;
    }

    return integral_part + '.' + fractional_part;
}

}

// ----------------------------------------------------------------------------
// This method has been simplified and only supports computing the square root of integer value
[[nodiscard]] std::string compute_square_root_digit_by_digit_method(std::integral auto value_, unsigned int precision_) {
    // NaN is a special case
    if (value_ == NAN) { // std::isfinite with integer is not mandatory in the standard
        return std::to_string(NAN);
    }

    // Cannot calculate the root of a negative number
    if (value_ < 0) {
        return std::to_string(NAN);
    }

    return details::compute_square_root_digit_by_digit_method(value_, precision_);
}

// ----------------------------------------------------------------------------

namespace details {

void compute_integral_part_of_square_root(std::ostream& stream_, std::integral auto value_, square_root_digits_generator& generator_) {
    const auto integer_values = split_integer_into_groups_of_2_digits(value_);

    const auto inputs
        = std::views::reverse(integer_values)
        | std::views::transform(std::ref(generator_));

    std::ranges::for_each(inputs, [&stream_](const auto x) {
        stream_ << x;
    });

    return;
}

void compute_fractional_part_of_square_root(std::ostream& stream_, square_root_digits_generator& generator_, std::stop_token stop_);

void compute_square_root_digit_by_digit_method(std::ostream& stream_, std::integral auto value_, std::stop_token stop_) {
    assert(value_ != NAN && value_ >= 0);

    // Early return optimization
    if (value_ == 0 || value_ == 1) {
        stream_ << value_;
        return;
    }

    square_root_digits_generator generator;

    compute_integral_part_of_square_root(stream_, value_, generator);

    // Early return optimization when the number is a perfect square
    if (!generator.has_next_digit()) {
        return;
    }

    stream_ << '.';

    compute_fractional_part_of_square_root(stream_, generator, stop_);
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

    details::compute_square_root_digit_by_digit_method(stream_, value_, stop_);
}

// ----------------------------------------------------------------------------
// This is a non protable version, but probably the fastest one
#ifdef __GNUC__
[[nodiscard]] inline double compute_square_root_assembly_method(double value_) {
    double result;
    asm("fldl %[n];"           // Load the operand n onto the FPU stack
        "fsqrt;"               // Perform square root operation on the top of the FPU stack
        "fstpl %[res];"        // Store the result from the FPU stack into the variable result
        : [res] "=m"(result)   // Output constraint for the result variable
        : [n] "m"(value_)      // Input constraint for the input variable n
    );
    return result;
}
#endif

#endif // SQUARE_ROOT_HPP
﻿// Output the square root of 42 in different ways

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cmath>
#include <concepts>
#include <format>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <numeric>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include "large_integer.h"

// ----------------------------------------------------------------------------
// Arbitrarily long definition of sqrt(42)
#define SQRT42 6.4807406984078602309659674360879966577052043070583465497113543978

// ----------------------------------------------------------------------------
// Constant expression of sqrt(42)
template <typename T> inline constexpr T sqrt42 = static_cast<T>(SQRT42);

// ----------------------------------------------------------------------------
// Print value to the console with the specified format using std::printf
template <typename T>
void print_using_std_printf(const char* format_, unsigned int counter_, T value_) {
    int n = std::printf(format_, counter_, value_);

    if (n < 0) {
        std::cout << "Error while printing value\n";
    }
}

namespace details {

template <std::floating_point T>
constexpr auto compute_square_root_exception(T value_, std::invocable<T> auto func_) -> T {
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
#if __cpp_lib_constexpr_cmath >= 202202L
constexpr
#endif
auto
compute_square_root_using_fractional_and_exponent_optimization(
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
auto compute_square_root_binary_search_method_fractional(T fractional_) -> T {
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
auto compute_square_root_binary_search_method(T value_) -> T {
    return details::compute_square_root_exception(value_, [](T value_) {
        return details::compute_square_root_using_fractional_and_exponent_optimization(
            value_,
            details::compute_square_root_binary_search_method_fractional<T>);
    });
}

// ----------------------------------------------------------------------------
// Overload for integral value
double compute_square_root_binary_search_method(std::integral auto value_) {
    return compute_square_root_binary_search_method<double>(value_);
}

// ----------------------------------------------------------------------------
// Test cases
TEST_CASE("compute_square_root_binary_search_method") {
    CHECK(compute_square_root_binary_search_method(0) == std::sqrt(0));
    CHECK(compute_square_root_binary_search_method(-0) == std::sqrt(-0));
    CHECK(compute_square_root_binary_search_method(1) == std::sqrt(1));
    CHECK(compute_square_root_binary_search_method(4) == std::sqrt(4));
    CHECK(compute_square_root_binary_search_method(2) == Catch::Approx(std::numbers::sqrt2).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_binary_search_method(780.14) == Catch::Approx(std::sqrt(780.14)).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_binary_search_method(0.5) == Catch::Approx(std::sqrt(0.5)).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_binary_search_method(42) == Catch::Approx(SQRT42 /*std::sqrt(42)*/).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_binary_search_method(1e-15) == Catch::Approx(std::sqrt(1e-15)).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_binary_search_method(1e-300) == Catch::Approx(std::sqrt(1e-300)).epsilon(std::numeric_limits<double>::epsilon()));
}

namespace details {

// ----------------------------------------------------------------------------
// Compute square root of the fractional part
template <std::floating_point T>
constexpr auto compute_square_root_heron_method_fractional(T fractional_) -> T {
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
auto compute_square_root_heron_method(T value_) -> T {
    return details::compute_square_root_exception(value_, [](T value_) {
        return details::
            compute_square_root_using_fractional_and_exponent_optimization(
                value_, details::compute_square_root_heron_method_fractional<T>);
    });
}

// ----------------------------------------------------------------------------
// Overload for integral value
double compute_square_root_heron_method(std::integral auto value_) {
    return compute_square_root_heron_method<double>(value_);
}

// ----------------------------------------------------------------------------
// Compute the square root using heron's method
template <std::floating_point T>
constexpr auto compute_square_root_heron_method_constexpr(T value_) -> T {
    return details::compute_square_root_exception(value_, [](T value_) {
        return details::compute_square_root_heron_method_fractional(value_);
    });
}

// ----------------------------------------------------------------------------
// Overload for integral value
constexpr double
compute_square_root_heron_method_constexpr(std::integral auto value_) {
    return compute_square_root_heron_method_constexpr<double>(value_);
}

// ----------------------------------------------------------------------------
// Test cases
TEST_CASE("compute_square_root_heron_method") {
    CHECK(compute_square_root_heron_method(0) == std::sqrt(0));
    CHECK(compute_square_root_heron_method(-0) == std::sqrt(-0));
    CHECK(compute_square_root_heron_method(1) == std::sqrt(1));
    CHECK(compute_square_root_heron_method(4) == std::sqrt(4));
    CHECK(compute_square_root_heron_method(2) == Catch::Approx(std::numbers::sqrt2).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_heron_method(780.14) == Catch::Approx(std::sqrt(780.14)).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_heron_method(0.5) == Catch::Approx(std::sqrt(0.5)).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_heron_method(42) == Catch::Approx(SQRT42 /*std::sqrt(42)*/).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_heron_method(1e-15) == Catch::Approx(std::sqrt(1e-15)).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_heron_method(2.2e-300) == Catch::Approx(std::sqrt(2.2e-300)).epsilon(std::numeric_limits<double>::epsilon()));
}

namespace details {

// ----------------------------------------------------------------------------
// Compute square root of the fractional part
template <std::floating_point T>
auto compute_square_root_bakhshali_method_fractional(T fractional_) -> T {
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
auto compute_square_root_bakhshali_method(T value_) -> T {
    return details::compute_square_root_exception(value_, [](T value_) {
        return details::
            compute_square_root_using_fractional_and_exponent_optimization(
                value_,
                details::compute_square_root_bakhshali_method_fractional<T>);
    });
}

// ----------------------------------------------------------------------------
// Overload for integral value
double compute_square_root_bakhshali_method(std::integral auto value_) {
    return compute_square_root_bakhshali_method<double>(value_);
}

// ----------------------------------------------------------------------------
// Test cases
TEST_CASE("compute_square_root_bakhshali_method") {
    CHECK(compute_square_root_bakhshali_method(0) == std::sqrt(0));
    CHECK(compute_square_root_bakhshali_method(-0) == std::sqrt(-0));
    CHECK(compute_square_root_bakhshali_method(1) == std::sqrt(1));
    CHECK(compute_square_root_bakhshali_method(4) == std::sqrt(4));
    CHECK(compute_square_root_bakhshali_method(2) == Catch::Approx(std::numbers::sqrt2).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_bakhshali_method(780.14) == Catch::Approx(std::sqrt(780.14)).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_bakhshali_method(0.5) == Catch::Approx(std::sqrt(0.5)).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_bakhshali_method(42) == Catch::Approx(SQRT42 /*std::sqrt(42)*/).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_bakhshali_method(1e-15) == Catch::Approx(std::sqrt(1e-15)).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_bakhshali_method(1e-300) == Catch::Approx(std::sqrt(1e-300)).epsilon(std::numeric_limits<double>::epsilon()));
    CHECK(compute_square_root_bakhshali_method(2.2e-300) == Catch::Approx(std::sqrt(2.2e-300)).epsilon(std::numeric_limits<double>::epsilon()));
}

// ----------------------------------------------------------------------------
// This method has been simplified and only supports computing the square root of integer value
std::string compute_square_root_digit_by_digit_method(std::integral auto value_, unsigned int max_precision_) {
    // NaN is a special case
    if (value_ == NAN) { // std::isfinite with integer is not mandatory in the standard
        return std::to_string(NAN);
    }

    // Cannot calculate the root of a negative number
    if (value_ < 0) {
        return std::to_string(NAN);
    }

    // Early return optimization
    if (value_ == 0 || value_ == 1) {
        return std::to_string(value_);
    }

    // Compute integral part of the square root
    std::vector<unsigned int> integer_values;

    auto residual = value_;
    while (residual > 0) {
        integer_values.emplace_back(static_cast<unsigned int>(residual % 100));
        residual /= 100;
    }

    large_integer remainder{ 0 };
    large_integer result{ 0 };

    auto compute_next_digit = [&remainder, &result](auto current_) {
        // find x * (20p + x) <= remainder*100+current
        large_integer current_remainder = remainder * 100 + current_;
        unsigned int x{ 0 };
        large_integer sum{ 0 };
        auto expanded_result = result * 20;
        while (true) {
            ++x;

            large_integer next_sum{ 0 };
            next_sum = (expanded_result + x);
            next_sum = next_sum * x;

            if (next_sum > current_remainder) {
                --x;
                break;
            }

            sum = next_sum;
        }

        assert(x < 10);

        result = result * 10 + x;
        remainder = current_remainder - sum;

        return x;
    };

    const auto integral_string_rng
        = std::views::reverse(integer_values)
        | std::views::transform(compute_next_digit)
        | std::views::transform([](auto x) {
        assert(x < 10);
        return static_cast<std::string::value_type>(x) + '0';
    });

    std::string result_string(std::begin(integral_string_rng), std::end(integral_string_rng));

    // Early return optimization when the number is a perfect square
    if (remainder == large_integer(0)) {
        return result_string;
    }

    // Adjust precision according to the number of integral digits
    auto precision = max_precision_ > result_string.length()
        ? max_precision_ - result_string.length()
        : 0;

    // Adjust capacity of result_string
    result_string.reserve(result_string.length() + precision + 1);

    if (precision > 0) {
        result_string += ".";
    }

    // Compute the fractional part
    constexpr unsigned int next_value = 0;

    while (precision > 0) {
        result_string +=
            static_cast<std::string::value_type>(compute_next_digit(next_value)) + '0';

        if (remainder == large_integer(0)) {
            break;
        }

        --precision;
    }

    // Round the last digit
    const auto rounding_digit = compute_next_digit(next_value);

    const double last_digit = result_string.back() - '0';
    const double rounded_last_digit = std::round(last_digit + (static_cast<double>(rounding_digit) / 10.0));

    if (rounded_last_digit >= 10) {
        result_string.back() = '0';
        bool carry = true;
        for (auto& digit
            : result_string
            | std::views::reverse
            | std::views::drop(1)
            | std::views::filter([](auto c) { return c != '.'; })) { // This assume a locale that split number using '.'
            if (digit == '9') {
                digit = '0';
            } else {
                digit += 1;
                carry = false;
                break;
            }
        }

        if (carry) {
            result_string.insert(std::begin(result_string), '1');
            // This assume a locale that split number using '.'
#if __cpp_lib_string_contains >= 202011L
            if (result_string.contains('.')) {
#else
            if (result_string.find('.') != std::string::npos) {
#endif
                result_string.pop_back();
            }
        }
    } else {
        result_string.back() = static_cast<std::string::value_type>(rounded_last_digit) + '0';
    }

    // trim 0s to the left
    // This assume a locale that split number using '.'
#if __cpp_lib_string_contains >= 202011L
    if (result_string.contains('.')) {
#else
    if (result_string.find('.') != std::string::npos) {
#endif
        auto index = result_string.length() - 1;
        while (index > 0) {
            if (result_string[index] == '0') {
                result_string.pop_back();
            } else {
                break;
            }
        }
    }

    // If there is no more fractional value, remove the '.'
    if (result_string.back() == '.') {
        result_string.pop_back();
    }

    return result_string;
}

TEST_CASE("compute_square_root_digit_by_digit_method") {
    using namespace std::string_literals;
    CHECK(compute_square_root_digit_by_digit_method(-1, 2) == "nan"s);
    CHECK(compute_square_root_digit_by_digit_method(0, 0) == "0"s);
    CHECK(compute_square_root_digit_by_digit_method(-0, 0) == "0"s);
    CHECK(compute_square_root_digit_by_digit_method(1, 0) == "1"s);
    CHECK(compute_square_root_digit_by_digit_method(4, 0) == "2"s);
    CHECK(compute_square_root_digit_by_digit_method(2, 32) == "1.4142135623730950488016887242097"s);
    CHECK(compute_square_root_digit_by_digit_method(42, 31) == "6.480740698407860230965967436088"s);
    CHECK(compute_square_root_digit_by_digit_method(42, 100) == "6.480740698407860230965967436087996657705204307058346549711354397809617377844044371400360906605610236"s);

    CHECK(compute_square_root_digit_by_digit_method(99, 1) == "10"s);
    CHECK(compute_square_root_digit_by_digit_method(99, 2) == "9.9"s);
    CHECK(compute_square_root_digit_by_digit_method(9999, 3) == "100"s);
    CHECK(compute_square_root_digit_by_digit_method(999999, 5) == "1000"s);
}

template <double Value> struct constexpr_type {
    static constexpr double value = Value;
};

template <double Value>
constexpr auto constexpr_type_v = constexpr_type<Value>::value;

// This is a non protable version, but probably the fastest one
#ifdef __GNUC__
inline double sqrt_asm(double value_) {
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

int main(int argc, const char* argv[]) {
    const int result = Catch::Session().run(argc, argv);

    // Adjust the number of digits to the number of significant digits for a long double
    std::cout << std::setprecision(std::numeric_limits<long double>::max_digits10);

    unsigned int counter = 0;
    std::cout << ++counter << ". Using a defined constant: " << SQRT42 << '\n';

    std::cout << ++counter << ". Using cout stream operator with a constexpr constant as an integer: " << sqrt42<int> << '\n';
    std::cout << ++counter << ". Using cout stream operator with a constexpr constant as a float: " << sqrt42<float> << '\n';
    std::cout << ++counter << ". Using cout stream operator with a constexpr constant as a double: " << sqrt42<double> << '\n';
    std::cout << ++counter << ". Using cout stream operator with a constexpr constant as a long double: " << sqrt42<long double> << '\n';

    print_using_std_printf("%d. Using std::printf with fix-point notation and float: %.9f\n", ++counter, sqrt42<float>); // This is useless as f consider the argument as a double
    print_using_std_printf("%d. Using std::printf with fix-point notation and double: %.17lf\n", ++counter, sqrt42<double>);
    print_using_std_printf("%d. Using std::printf with fix-point notation and long double: %.17Lf\n", ++counter, sqrt42<long double>);

    print_using_std_printf("%d. Using std::printf with scientific notation with float: %.9e\n", ++counter, sqrt42<float>); // This is useless as f consider the argument as a double
    print_using_std_printf("%d. Using std::printf with scientific notation with double: %.17le\n", ++counter, sqrt42<double>);
    print_using_std_printf("%d. Using std::printf with scientific notation with long double: %.17Le\n", ++counter, sqrt42<long double>);

    print_using_std_printf("%d. Using std::printf with hexadecimal notation with float: %a\n", ++counter, sqrt42<float>);
    print_using_std_printf("%d. Using std::printf with hexadecimal notation with double: %la\n", ++counter, sqrt42<double>);
    print_using_std_printf("%d. Using std::printf with hexadecimal notation with long double: %La\n", ++counter, sqrt42<long double>);

    std::cout << ++counter << ". Using stream operator with a std::sqrt(float): " << std::sqrt(42.0f) << '\n';
    std::cout << ++counter << ". Using stream operator with a std::sqrt(double): " << std::sqrt(42.0) << '\n';
    std::cout << ++counter << ". Using stream operator with a std::sqrt(long double): " << std::sqrt(42.0l) << '\n';
    std::cout << ++counter << ". Using stream operator with a std::sqrtf(float): " << std::sqrt(42.0f) << '\n';
    std::cout << ++counter << ". Using stream operator with a std::sqrtl(long double): " << std::sqrt(42.0l) << '\n';

    std::cout << std::format("{:}. Using std::format with fix-point notation and float: {:.9f}\n", ++counter, sqrt42<float>);
    std::cout << std::format("{:}. Using std::format with fix-point notation and double: {:.17f}\n", ++counter, sqrt42<double>);
    std::cout << std::format("{:}. Using std::format with fix-point notation and long double: {:.17f}\n", ++counter, sqrt42<long double>);

    std::cout << std::format("{:}. Using std::format with scientific notation and float: {:.9e}\n", ++counter, sqrt42<float>);
    std::cout << std::format("{:}. Using std::format with scientific notation and double: {:.17e}\n", ++counter, sqrt42<double>);
    std::cout << std::format("{:}. Using std::format with scientific notation and long double: {:.17e}\n", ++counter, sqrt42<long double>);

    std::cout << std::format("{:}. Using std::format with hexadecimal notation and float: {:a}\n", ++counter, sqrt42<float>);
    std::cout << std::format("{:}. Using std::format with hexadecimal notation and double: {:a}\n", ++counter, sqrt42<double>);
    std::cout << std::format("{:}. Using std::format with hexadecimal notation and long double: {:a}\n", ++counter, sqrt42<long double>);

    std::cout << ++counter << ". Using std::pow: " << std::pow(42.0l, 0.5l) << '\n';

#ifdef __GNUC__
    std::cout << ++counter << ". Using assembly fsqrt: " << sqrt_asm(42.0) << '\n';
#endif

    // Pass the value to a template expression as a proof of a constant expression
    std::cout << ++counter << ". Using custom function using constexpr Heron's method: " <<
#if __cpp_nontype_template_args >= 201911L
        constexpr_type_v<compute_square_root_heron_method_constexpr(42)>
#else
        compute_square_root_heron_method_constexpr(42)
#endif
        << '\n';

    std::cout << ++counter << ". Using custom function using Newton's method: " << compute_square_root_binary_search_method(42) << '\n';
    std::cout << ++counter << ". Using custom function using Heron's method: " << compute_square_root_heron_method(42) << '\n';
    std::cout << ++counter << ". Using custom function using Bakhshali's method: " << compute_square_root_bakhshali_method(42) << '\n';
    std::cout << ++counter << ". Using infinite digits (only show 1k): " << compute_square_root_digit_by_digit_method(42, 1'000) << '\n';

    return result;
}
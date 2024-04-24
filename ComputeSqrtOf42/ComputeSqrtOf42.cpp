// Output the square root of 42 in different ways

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <format>
#include <iostream>
#include <iomanip>
#include <numbers>
#include <numeric>
#include <ranges>
#include <span>
#include <string>
#include <vector>

// TODO: Replace catch2 with a custom test framework
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

// ----------------------------------------------------------------------------
// Arbitrarily long definition of sqrt(42)
#define SQRT42 6.480740698407860230965967436087996657705204307058346549711354397809617377844044371400360906605610236

// ----------------------------------------------------------------------------
// Constant expression of sqrt(42)
template<typename T>
inline constexpr T sqrt42 = static_cast<T>(SQRT42);

// ----------------------------------------------------------------------------
// Print value to the console with the specified format using std::printf
template<typename T>
void print_using_std_printf(const char* format_, unsigned int counter_, T value_)
{
    int n = std::printf(format_, counter_, value_);

    if (n < 0)
    {
        std::cout << "Error while printing value\n";
    }
}

namespace details {

template<std::floating_point T>
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
// Split value into its fractional and exponent part with the exponent always even
// fractional part is (-2, -0.25], [0.25, 2)
// return [fractional, exponent]
template<typename T>
#if __cpp_lib_constexpr_cmath >= 202202L
constexpr
#endif
std::pair<T, int> split_into_fractional_and_even_exponent(T value_)
{
    int exponent{};
    T fractional = std::frexp(value_, &exponent);

    // Check is exponent is odd by looking at the first bit
    if (exponent & 1) {
        if (exponent < 0) {
            fractional /= 2;
            ++exponent;
        }
        else {
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
template<std::floating_point T>
#if __cpp_lib_constexpr_cmath >= 202202L
constexpr
#endif
auto compute_square_root_using_fractional_and_exponent_optimization(T value_, std::invocable<T> auto func_) -> T {
    // Split the problem into the fractional and the exponent parts
    const auto [fractional, exponent] = details::split_into_fractional_and_even_exponent(value_);

    // Early return for 0 or 1
    if (fractional == T{ 0 } || fractional == T{ 1 }) {
        return std::ldexp(fractional, exponent / 2);
    }

    const auto result = func_(fractional);

    // Reconstruct the result
    return std::ldexp(result, exponent / 2);
}

}

namespace details {

// ----------------------------------------------------------------------------
// Compute square root of the fractional part
template<std::floating_point T>
auto compute_square_root_binary_search_method_fractional(T fractional_) -> T {
    assert(0 <= fractional_ && fractional_ < 2);

    // Search from zero to fractional_
    // If fractional_ is less than 1, then the result will be greater than fractional_, hence the search is from 0 to fractional_ + 1
    // Since fractional_ is between [0.25,2) the sqrt will be between 0 and, at worst, sqrt(2), hence the maximum is fixed to sqrt(2)
    T left = 0;
    T right = fractional_ >= 1 ? std::min(fractional_, std::numbers::sqrt2_v<T>) : std::min(fractional_+1, std::numbers::sqrt2_v<T>);
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
        }
        else {
            right = middle;
        }

        old = middle;
    }
}

}

// ----------------------------------------------------------------------------
// Compute the square root using a binary search
template<std::floating_point T>
auto compute_square_root_binary_search_method(T value_) -> T {
    return details::compute_square_root_exception(value_, [](T value_) {
        return details::compute_square_root_using_fractional_and_exponent_optimization(value_, details::compute_square_root_binary_search_method_fractional<T>);
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
    REQUIRE(compute_square_root_binary_search_method(0) == std::sqrt(0));
    REQUIRE(compute_square_root_binary_search_method(-0) == std::sqrt(-0));
    REQUIRE(compute_square_root_binary_search_method(1) == std::sqrt(1));
    REQUIRE(compute_square_root_binary_search_method(4) == std::sqrt(4));
    REQUIRE_THAT(compute_square_root_binary_search_method(2), Catch::Matchers::WithinAbs(std::sqrt(2), std::numeric_limits<double>::epsilon()));
    REQUIRE_THAT(compute_square_root_binary_search_method(780.14), Catch::Matchers::WithinAbs(std::sqrt(780.14), std::numeric_limits<double>::epsilon()));
    REQUIRE_THAT(compute_square_root_binary_search_method(0.5), Catch::Matchers::WithinAbs(std::sqrt(0.5), std::numeric_limits<double>::epsilon()));
    
    REQUIRE_THAT(compute_square_root_binary_search_method(42), Catch::Matchers::WithinAbs(SQRT42/*std::sqrt(42)*/, std::numeric_limits<double>::epsilon()));

    REQUIRE_THAT(compute_square_root_binary_search_method(1e-15), Catch::Matchers::WithinAbs(std::sqrt(1e-15), std::numeric_limits<double>::epsilon()));
    REQUIRE_THAT(compute_square_root_binary_search_method(1e-300), Catch::Matchers::WithinAbs(std::sqrt(1e-300), std::numeric_limits<double>::epsilon()));
}

namespace details {

// ----------------------------------------------------------------------------
// Compute square root of the fractional part
template<std::floating_point T>
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

}

// ----------------------------------------------------------------------------
// Compute the square root using heron's method
template<std::floating_point T>
auto compute_square_root_heron_method(T value_) -> T {
    return details::compute_square_root_exception(value_, [](T value_) {
            return details::compute_square_root_using_fractional_and_exponent_optimization(value_, details::compute_square_root_heron_method_fractional<T>);
        });
}

// ----------------------------------------------------------------------------
// Overload for integral value
double compute_square_root_heron_method(std::integral auto value_) {
    return compute_square_root_heron_method<double>(value_);
}

// ----------------------------------------------------------------------------
// Compute the square root using heron's method
template<std::floating_point T>
constexpr auto compute_square_root_heron_method_constexpr(T value_) -> T {
    return details::compute_square_root_exception(value_, [](T value_) {
            return details::compute_square_root_heron_method_fractional(value_);
        });
}

// ----------------------------------------------------------------------------
// Overload for integral value
constexpr double compute_square_root_heron_method_constexpr(std::integral auto value_) {
    return compute_square_root_heron_method_constexpr<double>(value_);
}

// ----------------------------------------------------------------------------
// Test cases
TEST_CASE("compute_square_root_heron_method") {
    CHECK(compute_square_root_heron_method(0) == std::sqrt(0));
    CHECK(compute_square_root_heron_method(-0) == std::sqrt(-0));
    CHECK(compute_square_root_heron_method(1) == std::sqrt(1));
    CHECK(compute_square_root_heron_method(4) == std::sqrt(4));
    CHECK_THAT(compute_square_root_heron_method(2), Catch::Matchers::WithinAbs(std::sqrt(2), std::numeric_limits<double>::epsilon()));
    CHECK_THAT(compute_square_root_heron_method(780.14), Catch::Matchers::WithinAbs(std::sqrt(780.14), std::numeric_limits<double>::epsilon()));
    CHECK_THAT(compute_square_root_heron_method(0.5), Catch::Matchers::WithinAbs(std::sqrt(0.5), std::numeric_limits<double>::epsilon()));

    CHECK_THAT(compute_square_root_heron_method(42), Catch::Matchers::WithinAbs(SQRT42/*std::sqrt(42)*/, std::numeric_limits<double>::epsilon()));

    CHECK_THAT(compute_square_root_heron_method(1e-15), Catch::Matchers::WithinAbs(std::sqrt(1e-15), std::numeric_limits<double>::epsilon()));
    CHECK_THAT(compute_square_root_heron_method(2.2e-300), Catch::Matchers::WithinAbs(std::sqrt(2.2e-300), std::numeric_limits<double>::epsilon()));
}

namespace details {

// ----------------------------------------------------------------------------
// Compute square root of the fractional part
template<std::floating_point T>
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

}

// ----------------------------------------------------------------------------
// Compute the square root using bakhshali's method
template<std::floating_point T>
auto compute_square_root_bakhshali_method(T value_) -> T {
    return details::compute_square_root_exception(value_, [](T value_) {
            return details::compute_square_root_using_fractional_and_exponent_optimization(value_, details::compute_square_root_bakhshali_method_fractional<T>);
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
    CHECK_THAT(compute_square_root_bakhshali_method(2), Catch::Matchers::WithinAbs(std::sqrt(2), std::numeric_limits<double>::epsilon()));
    CHECK_THAT(compute_square_root_bakhshali_method(780.14), Catch::Matchers::WithinAbs(std::sqrt(780.14), std::numeric_limits<double>::epsilon()));
    CHECK_THAT(compute_square_root_bakhshali_method(0.5), Catch::Matchers::WithinAbs(std::sqrt(0.5), std::numeric_limits<double>::epsilon()));

    CHECK_THAT(compute_square_root_bakhshali_method(42), Catch::Matchers::WithinAbs(SQRT42/*std::sqrt(42)*/, std::numeric_limits<double>::epsilon()));

    CHECK_THAT(compute_square_root_bakhshali_method(1e-15), Catch::Matchers::WithinAbs(std::sqrt(1e-15), std::numeric_limits<double>::epsilon()));
    CHECK_THAT(compute_square_root_bakhshali_method(1e-300), Catch::Matchers::WithinAbs(std::sqrt(1e-300), std::numeric_limits<double>::epsilon()));
    CHECK_THAT(compute_square_root_bakhshali_method(2.2e-300), Catch::Matchers::WithinAbs(std::sqrt(2.2e-300), std::numeric_limits<double>::epsilon()));
}

// ----------------------------------------------------------------------------
// Forward declaration
class large_integer;
constexpr large_integer from_string(const std::string& str_);


// ----------------------------------------------------------------------------
// Large integer to allow infinitely large integer number
class large_integer {
public:
    using underlying_type = std::uint32_t;
    using extended_type = std::uint64_t;
    using signed_extended_type = std::int64_t;

    using collection_type = std::vector<underlying_type>;

    static constexpr auto nb_extended_type_bits = sizeof(underlying_type) * 8;
    static constexpr const extended_type base = extended_type{ 1 } << nb_extended_type_bits;

    constexpr large_integer() : large_integer(false, { 0 } ) {}

    constexpr large_integer(std::integral auto value_) : large_integer(value_ < 0, to_data_collection(value_)) {}

    constexpr large_integer(const char* str_) : large_integer(from_string(str_)) {}
    constexpr large_integer(const std::string& str_) : large_integer(from_string(str_)) {}

    [[nodiscard]] constexpr large_integer operator-() const {
        return { !sign, data };
    }

    [[nodiscard]] constexpr large_integer operator+(const large_integer& other_) const {
        return large_integer_data_ref(*this) + large_integer_data_ref(other_);
    }

    [[nodiscard]] constexpr large_integer operator-(const large_integer& other_) const {
        return large_integer_data_ref(*this) - large_integer_data_ref(other_);
    }

    [[nodiscard]] constexpr large_integer operator*(const large_integer& other_) const {
        return large_integer_data_ref(*this) * large_integer_data_ref(other_);
    }

    [[nodiscard]] constexpr std::strong_ordering operator<=>(const large_integer& other_) const {
        return large_integer_data_ref(*this) <=> large_integer_data_ref(other_);
    }

    [[nodiscard]] constexpr std::strong_ordering operator<=>(std::integral auto rhs_) const {
        return *this <=> large_integer(rhs_);
    }

    bool operator==(const large_integer& rhs_) const {
        return (*this <=> large_integer(rhs_)) == std::strong_ordering::equal;
    }

    bool operator==(std::integral auto rhs_) const {
        return (*this <=> large_integer(rhs_)) == std::strong_ordering::equal;
    }

private:
    friend constexpr large_integer from_string(const std::string& str_);
    friend constexpr std::string to_string(const large_integer& value_);

    class large_integer_data_ref {
    public:
        constexpr large_integer_data_ref(const large_integer& other_) : large_integer_data_ref(other_.sign, std::cref(other_.data)) {}
        constexpr large_integer_data_ref(bool sign_, const std::vector<underlying_type>& data_) : sign(sign_), data(std::cref(data_)) {}

        [[nodiscard]] constexpr large_integer operator+(const large_integer_data_ref& other_) const {
            // Enforce signs to be the same
            if (sign != other_.sign) {
                return *this - large_integer_data_ref(!other_.sign, other_.data.get());
            }

            // Enforce lhs to be larger than rhs
            if (*this < other_) {
                return other_ + *this;
            }

            bool result_sign = sign;
            auto result_data = add_large_unsigned_integer_sorted(data.get(), other_.data.get());

            return cleanup({ result_sign, std::move(result_data) });
        }

        [[nodiscard]] constexpr large_integer operator-(const large_integer_data_ref& other_) const {
            // Enforce signs to be the same
            if (sign != other_.sign) {
                return *this + large_integer_data_ref(!other_.sign, other_.data.get());
            }

            // Enforce lhs to be larger than rhs
            if (*this < other_) {
                auto result = other_ - *this;
                return { !result.sign, std::move(result.data) };
            }

            bool result_sign = sign;
            auto result_data = subtract_large_unsigned_integer_sorted(data.get(), other_.data.get());

            return cleanup({ result_sign, std::move(result_data) });
        }

        [[nodiscard]] constexpr large_integer operator*(const large_integer_data_ref& other_) const {
            // Enforce signs to be the same
            if (sign != other_.sign) {
                bool result_sign = true;
                auto result = *this * large_integer_data_ref(!other_.sign, other_.data.get());
                return { result_sign, std::move(result.data) };
            }

            // Enforce lhs to be larger than rhs
            if (*this < other_) {
                return other_ * *this;
            }

            bool result_sign = false;
            auto result_data = multiply_large_unsigned_integer_sorted(data.get(), other_.data.get());

            return cleanup({ result_sign, std::move(result_data) });
        }

        [[nodiscard]] constexpr std::strong_ordering operator<=>(const large_integer_data_ref& other_) const {
            if (sign != other_.sign) {
                return sign ? std::strong_ordering::greater : std::strong_ordering::less;
            }

            return compare_large_unsigned_integer(data, other_.data);
        }

    private:
        bool sign{};
        std::reference_wrapper<const collection_type> data;
    };

    constexpr large_integer(bool sign_, std::vector<underlying_type> data_) : sign(sign_), data(std::move(data_)) {}

    [[nodiscard]] static constexpr collection_type to_data_collection(std::integral auto value_)
    {
        std::vector<underlying_type> data;
        if constexpr (std::signed_integral<decltype(value_)>) {
            value_ = std::abs(value_);
        }

        if constexpr (sizeof(decltype(value_)) > sizeof(underlying_type)) {
            constexpr auto max_value = std::numeric_limits<underlying_type>::max();
            while (value_ > max_value) {
                // Keep only the lower part that can be stored in underlying_type
                data.emplace_back(static_cast<underlying_type>(value_));
                // Compute the overflow that cannot be stored
                value_ >>= nb_extended_type_bits;
            }
        }

        data.emplace_back(static_cast<underlying_type>(value_));

        return data;
    }

    [[nodiscard]] static constexpr collection_type add_large_unsigned_integer_sorted(const collection_type& lhs_, const collection_type& rhs_) {
        assert(compare_large_unsigned_integer(lhs_, rhs_) == std::strong_ordering::equal || compare_large_unsigned_integer(lhs_, rhs_) == std::strong_ordering::greater);

        std::vector<underlying_type> result_data(lhs_.size() + 1, 0);

        size_t index = 0;
        for (; index < rhs_.size(); ++index) {
            const extended_type lhs_value = lhs_[index];
            const extended_type rhs_value = rhs_[index];
            const extended_type old_result = result_data[index];
            const extended_type sum = lhs_value + rhs_value + old_result;

            // Keep only the lower part that can be stored in underlying_type
            result_data[index] = static_cast<underlying_type>(sum);

            // Compute the overflow that cannot be stored
            result_data[index + 1] = sum >> nb_extended_type_bits;
        }

        // Expand the overflow
        for (; index < lhs_.size(); ++index) {
            const extended_type lhs_value = lhs_[index];
            const extended_type old_result = result_data[index];
            const extended_type sum = lhs_value + old_result;

            // Keep only the lower part that can be stored in underlying_type
            result_data[index] = static_cast<underlying_type>(sum);

            // Compute the overflow that cannot be stored
            result_data[index + 1] = sum >> nb_extended_type_bits;
        }

        return result_data;
    }

    [[nodiscard]] static constexpr collection_type subtract_large_unsigned_integer_sorted(const collection_type& lhs_, const collection_type& rhs_) {
        assert(compare_large_unsigned_integer(lhs_, rhs_) == std::strong_ordering::equal || compare_large_unsigned_integer(lhs_, rhs_) == std::strong_ordering::greater);

        std::vector<underlying_type> result_data;
        result_data.reserve(lhs_.size());

        // Subtract every digit of rhs from the corresponding lhs
        bool carry = false;
        const size_t min_size = rhs_.size();
        size_t index = 0;
        for (; index < min_size; ++index) {
            const signed_extended_type lhs_value = lhs_[index];
            const signed_extended_type rhs_value = rhs_[index];

            signed_extended_type value = lhs_value;
            if (carry) {
                --value;
            }

            if (value < rhs_value) {
                value += base;
                carry = true;
            }
            else {
                carry = false;
            }

            extended_type diff = value - rhs_value;
            assert(diff <= std::numeric_limits<underlying_type>::max());
            result_data.emplace_back(static_cast<underlying_type>(diff));
        }

        // Extend the carry to the rest of lhs
        for (; index < lhs_.size(); ++index) {
            const signed_extended_type lhs = lhs_[index];
            signed_extended_type value = lhs;
            if (carry) {
                --value;
            }

            if (value < 0) {
                value += base;
                carry = true;
            }
            else {
                carry = false;
            }

            assert(value <= std::numeric_limits<underlying_type>::max());
            result_data.emplace_back(static_cast<underlying_type>(value));
        }

        return result_data;
    }

    // TODO: Use Karatsuba algorithm instead of the naive implementation
    // TODO: Use Toom-Cook algorithm
    // TODO: Use Schönhage–Strassen algorithm
    [[nodiscard]] static constexpr collection_type multiply_large_unsigned_integer_sorted(const collection_type& lhs_, const collection_type& rhs_) {
        assert(compare_large_unsigned_integer(lhs_, rhs_) == std::strong_ordering::equal || compare_large_unsigned_integer(lhs_, rhs_) == std::strong_ordering::greater);

        collection_type result_data(lhs_.size() * rhs_.size() + 1, 0);

        // Multiply each digit of rhs with each digit of lhs
        size_t result_index = 0;
        for (size_t rhs_index = 0; rhs_index < rhs_.size(); ++rhs_index) {
            extended_type overflow{ 0 };
            result_index = rhs_index;
            for (const extended_type lhs_value : lhs_) {
                const extended_type rhs_value = rhs_[rhs_index];
                const extended_type old_result = result_data[result_index];

                extended_type value = lhs_value * rhs_value + overflow + old_result;
                result_data[result_index] = static_cast<underlying_type>(value);

                overflow = value >> nb_extended_type_bits;

                ++result_index;
            }

            result_data[result_index] = static_cast<underlying_type>(overflow);
        }

        return result_data;
    }

    [[nodiscard]] static constexpr std::strong_ordering compare_large_unsigned_integer(const collection_type& lhs_, const collection_type& rhs_)
    {
        if (lhs_.size() != rhs_.size()) {
            return (lhs_.size() < rhs_.size()) ? std::strong_ordering::less : std::strong_ordering::greater;
        }

#if __cpp_lib_ranges_zip >= 202110L
        for (const auto& [lhs, rhs] : std::views::zip(lhs_, rhs_) | std::views::reverse) {
            if (lhs != rhs) {
                return (lhs < rhs) ? std::strong_ordering::less : std::strong_ordering::greater;
            }
        }
#else
        auto [it_lhs, it_rhs] = std::ranges::mismatch(lhs_ | std::views::reverse, rhs_ | std::views::reverse);
        if (it_lhs != lhs_.rend()) {
            return (*it_lhs < *it_rhs) ? std::strong_ordering::less : std::strong_ordering::greater;
        }
#endif

        return std::strong_ordering::equal;
    }

    [[nodiscard]] static constexpr large_integer trim_upper_zeros(large_integer&& result_) {
        size_t index = result_.data.size();
        while (index > 1 && result_.data[--index] == 0) {
            result_.data.pop_back();
        }

        return result_;
    }

    // Handle the -0 case
    static constexpr large_integer fix_minus_zeros(large_integer&& result_) {
        if (result_.data == collection_type{ 0 }) {
            result_.sign = false;
        }

        return std::move(result_);
    }

    [[nodiscard]] static constexpr large_integer cleanup(large_integer&& result_) {
        result_ = trim_upper_zeros(std::move(result_));

        // Free unused data
        result_.data.shrink_to_fit();

        result_ = fix_minus_zeros(std::move(result_));

        return result_;
    }

    bool sign{};
    collection_type data;
};

large_integer operator+(const large_integer& lhs_, std::integral auto rhs_) {
    return lhs_ + large_integer(rhs_);
}

large_integer operator+(std::integral auto lhs_, const large_integer& rhs_) {
    return large_integer(lhs_) + rhs_;
}

large_integer operator-(const large_integer& lhs_, std::integral auto rhs_) {
    return lhs_ - large_integer(rhs_);
}

large_integer operator-(std::integral auto lhs_, const large_integer& rhs_) {
    return large_integer(lhs_) - rhs_;
}

large_integer operator*(const large_integer& lhs_, std::integral auto rhs_) {
    return lhs_ * large_integer(rhs_);
}

large_integer operator*(std::integral auto lhs_, const large_integer& rhs_) {
    return large_integer(lhs_) * rhs_;
}

std::strong_ordering operator<=>(const large_integer& lhs_, std::integral auto rhs_) {
    return lhs_ <=> large_integer(rhs_);
}

std::strong_ordering operator<=>(std::integral auto lhs_, const large_integer& rhs_) {
    return large_integer(lhs_) <=> rhs_;
}

bool operator==(const large_integer& lhs_, std::integral auto rhs_) {
    return lhs_ == large_integer(rhs_);
}

bool operator==(std::integral auto lhs_, const large_integer& rhs_) {
    return large_integer(lhs_) == rhs_;
}

namespace details {

// A function to perform division of large numbers
[[nodiscard]] constexpr std::string divide_integer_as_string_by_integer(const std::string& number, large_integer::extended_type divisor) {
    // As result can be very large store it in string
    std::string ans;

    // Find prefix of number that is larger
    // than divisor.
    size_t idx = 0;
    large_integer::extended_type temp = number[idx] - '0';
    while (idx < (number.size() - 1) && temp < divisor)
        temp = temp * 10 + (number[++idx] - '0');

    // Repeatedly divide divisor with temp. After
    // every division, update temp to include one
    // more digit.
    while ((number.size() - 1) > idx) {
        // Store result in answer i.e. temp / divisor
        ans += static_cast<char>(temp / divisor) + '0';

        // Take next digit of number
        temp = (temp % divisor) * 10 + number[++idx] - '0';
    }

    ans += static_cast<char>(temp / divisor) + '0';

    // else return ans
    return ans;
}

[[nodiscard]] constexpr large_integer::extended_type modulo_integer_as_string_by_integer(const std::string& num, large_integer::extended_type a)
{
    // Initialize result
    large_integer::extended_type res = 0;

    // One by one process all digits of 'num'
    for (auto i : num)
        res = (res * 10 + (i - '0')) % a;

    return res;
}

}

[[nodiscard]] constexpr large_integer from_string(const std::string& str_) {
    if (str_.empty()) {
        // TODO: This should probably throw
        return {};
    }

    bool sign = false;
    auto begin = std::begin(str_);
    if (str_[0] == '-') {
        sign = true;
        ++begin;
        // TODO: Add error management
        // this will throw if the number is ill-formed with "-"
    }

    std::string number(begin, std::end(str_));

    std::vector<large_integer::underlying_type> data;
    do {
        data.emplace_back(static_cast<large_integer::underlying_type>(details::modulo_integer_as_string_by_integer(number, large_integer::base)));
        number = details::divide_integer_as_string_by_integer(number, large_integer::base);
    } while (number != "0");

    return { sign, data };
}

std::istream& operator>>(std::istream& stream_, large_integer& value_) {
    // Assume that the rdbuf exist and that the number is fully contains in the buffer
    assert(stream_.rdbuf() != nullptr);
    const std::streamsize size = stream_.rdbuf()->in_avail();
    std::string number(size, 0);
    stream_.read(number.data(), size);

    value_ = from_string(number);

    return stream_;
}

namespace details {

// Function to add two strings representing large integers
[[nodiscard]] constexpr std::string add_integers_as_string(const std::string& lhs_, const std::string& rhs_) {
    assert(!lhs_.empty() && !rhs_.empty());

    if (lhs_.empty() && rhs_.empty()) {
        return "0";
    }

    if (lhs_.empty()) {
        return rhs_;
    }

    if (rhs_.empty()) {
        return lhs_;
    }

    std::string sum;
    large_integer::underlying_type carry = 0;
    auto iter1 = lhs_.rbegin();
    auto iter2 = rhs_.rbegin();

    while (iter1 != lhs_.rend() || iter2 != rhs_.rend() || carry > 0) {
        large_integer::underlying_type digit1 = 0;
        if (iter1 != lhs_.rend()) {
            digit1 = *iter1 - '0';
            ++iter1;
        }

        large_integer::underlying_type digit2 = 0;
        if (iter2 != rhs_.rend()) {
            digit2 = *iter2 - '0';
            ++iter2;
        }

        large_integer::underlying_type digitSum = digit1 + digit2 + carry;
        carry = digitSum / 10;
        sum = std::to_string(digitSum % 10) + sum;
    }

    return sum;
}

// Function to multiply a string representing a large integer by an integer
[[nodiscard]] constexpr std::string multiply_integer_as_string_by_integer(const std::string& num_, large_integer::extended_type factor_) {
    std::string result;
    large_integer::extended_type carry = 0;
    for (auto digit_str : std::views::reverse(num_)) {
        large_integer::extended_type digit = digit_str - '0';
        large_integer::extended_type product = digit * factor_ + carry;
        carry = product / 10;
        result = std::to_string(product % 10) + result;
    }

    if (carry > 0) {
        result = std::to_string(carry) + result;
    }

    return result;
}

// Function to concatenate the vector elements into a single large integer represented as a string
[[nodiscard]] constexpr std::string concatenate_integers_to_string(const std::vector<large_integer::underlying_type>& data_) {
    std::string result = "0";
    for (size_t i = 0; i < data_.size(); ++i) {
        // Multiply the current element by the large_integer base
        std::string temp = std::to_string(data_[i]);
        for (size_t j = 0; j < i; ++j) {
            temp = multiply_integer_as_string_by_integer(temp, large_integer::base);
        }
        // Add the current number to the result
        result = add_integers_as_string(result, temp);
    }
    return result;
}

}

[[nodiscard]] constexpr std::string to_string(const large_integer& value_) {
    auto result = details::concatenate_integers_to_string(value_.data);

    if (value_.sign) {
        return '-' + result;
    }

    return result;
}

std::ostream& operator<<(std::ostream& stream_, const large_integer& value_) {
    stream_ << to_string(value_);
    return stream_;
}

TEST_CASE("large_integer") {
    using namespace std::string_literals;
    CHECK(to_string(large_integer(1)) == "1"s);
    CHECK(to_string(large_integer(-1)) == "-1"s);
    CHECK(to_string(large_integer(123456789012L)) == "123456789012"s);
    CHECK(to_string(large_integer(-123456789012L)) == "-123456789012"s);

    CHECK((large_integer(123456789011L) < large_integer(1L)) == false);
    CHECK((large_integer(1L) < large_integer(123456789012L)) == true);
    CHECK((large_integer(123456789012L) < large_integer(123456789011L)) == false);
    CHECK((large_integer(123456789012L) < large_integer(123456789012L)) == false);
    CHECK((large_integer(123456789011L) < large_integer(123456789012L)) == true);

    CHECK(to_string(large_integer(123456789012L) + large_integer(123456789012L) ) == "246913578024"s);
    CHECK(to_string(large_integer(-123456789012L) + large_integer(-123456789012L)) == "-246913578024"s);
    CHECK(to_string(large_integer(123456789012L) + large_integer(-123456789012L)) == "0"s);
    CHECK(to_string(large_integer(-123456789012L) + large_integer(123456789012L)) == "0"s);
    CHECK(to_string(large_integer(123456789012L) + large_integer(-123456789000L)) == "12"s);
    CHECK(to_string(large_integer(246913578024L) + large_integer(-123456789012L)) == "123456789012"s);
    CHECK(to_string(large_integer(-246913578024L) + large_integer(123456789012L)) == "-123456789012"s);
    CHECK(to_string(large_integer(-123456789012L) + large_integer(246913578024L)) == "123456789012"s);
    CHECK(to_string(large_integer(123456789012L) + large_integer(-246913578024L)) == "-123456789012"s);

    CHECK(to_string(large_integer("42010168383160134110440665745547766649977556245")) == "42010168383160134110440665745547766649977556245"s);
    CHECK(to_string(large_integer("-42010168383160134110440665745547766649977556245")) == "-42010168383160134110440665745547766649977556245"s);

    CHECK(to_string(large_integer("42010168383160134110440665745547766649977556245") - large_integer("42010168383160134110440665745547766649977556200")) == "45"s);

    CHECK(to_string(large_integer(246913578024L) * large_integer(123456789012L)) == "30483157506306967872288"s);
    CHECK(to_string(large_integer(-246913578024L) * large_integer(-123456789012L)) == "30483157506306967872288"s);
    CHECK(to_string(large_integer(246913578024L) * large_integer(-123456789012L)) == "-30483157506306967872288"s);
    CHECK(to_string(large_integer(-246913578024L) * large_integer(123456789012L)) == "-30483157506306967872288"s);
    CHECK(to_string(large_integer(-123456789012L) * large_integer(246913578024L)) == "-30483157506306967872288"s);
    CHECK(to_string(large_integer(123456789012L) * large_integer(-246913578024L)) == "-30483157506306967872288"s);

    CHECK(large_integer("42010168383160134110440665745547766649977556245") * large_integer("1234567890987654321") == large_integer("51864404980834242630409449768792397904982098404496001028394784645"));

    CHECK(large_integer("6779575297923493247898029418281817537676227380624747815049013732411535947860165631075856579568246233910811591607874120563664388642279371457390259857568960958935772908009048011104104746617436179252684469483776429833549880669503680760948677500")
        - large_integer("6480740698407860230965967436087996657705204307058346549711354397809617377844044371400360906605610235675450542097411694335491913404906608688945818961664673951305585227822636095668822680668761521776633672599142812990432160139844957280499363525")
        == large_integer("298834599515633016932061982193820879971023073566401265337659334601918570016121259675495672962635998235361049510462426228172475237372762768444440895904287007630187680186411915435282065948674657476050796884633616843117720529658723480449313975"));

}

// TODO: Generalize compute_square_root_digit_by_digit_method to handle both integers and floating point values
// This new version should return a large_floating_point instead of a string

// ----------------------------------------------------------------------------
// This method has been simplified and only supports computing the square root of integer value
std::string compute_square_root_digit_by_digit_method(std::integral auto value_, unsigned int max_precision_) {
    // NaN is a special case
    if (value_ == NAN) {    // std::isfinite with integer is not mandatory
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

    // TODO: move this lambda to a function and return a structure that includes the digit, the remainder and the result

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

    const auto integral_string_rng =
        std::views::reverse(integer_values) |
        std::views::transform(compute_next_digit) |
        std::views::transform([](auto x) { assert(x < 10); return static_cast<std::string::value_type>(x) + '0'; });

    std::string result_string(std::begin(integral_string_rng), std::end(integral_string_rng));

    // Early return optimization when the number is a perfect square
    if (remainder == large_integer(0)) {
        return result_string;
    }

    // Adjust precision according to the number of integral digits
    auto precision = max_precision_ > result_string.length() ? max_precision_ - result_string.length() : 0;

    // Adjust capacity of result_string
    result_string.reserve(result_string.length() + precision + 1);

    if (precision > 0) {
        result_string += ".";
    }

    // Compute the fractional part
    constexpr unsigned int next_value = 0;

    while (precision > 0) {
        result_string += static_cast<std::string::value_type>(compute_next_digit(next_value)) + '0';

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
        for (auto& digit : result_string | std::views::reverse | std::views::drop(1) | std::views::filter([](auto c) { return c != '.'; })) {
            if (digit == '9') {
                digit = '0';
            }
            else {
                digit += 1;
                carry = false;
                break;
            }
        }

        if (carry) {
            result_string.insert(std::begin(result_string), '1');
            const auto it = std::find(std::begin(result_string), std::end(result_string), '.');
            if (it != std::end(result_string)) {
                result_string.pop_back();
            }
        }
    }
    else {
        result_string.back() = static_cast<std::string::value_type>(rounded_last_digit) + '0';
    }

    // trim 0s to the left
    if (result_string.contains('.')) {
        auto index = result_string.length() - 1;
        while (index > 0) {
            if (result_string[index] == '0') {
                result_string.pop_back();
            }
            else {
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


template<double Value>
struct constexpr_type {
    static constexpr double value = Value;
};

template<double Value>
constexpr auto constexpr_type_v = constexpr_type<Value>::value;


int main(int argc, const char* argv[])
{
    int result = Catch::Session().run(argc, argv);

    // Adjust the number of digits to the number of significant digits for a long double
    std::cout << std::setprecision(std::numeric_limits<long double>::max_digits10);

    unsigned int counter = 0;
    std::cout << ++counter << ". Using defined constant: " << SQRT42 << '\n';

    std::cout << ++counter << ". Using stream operator with a constexpr constant as an integer: " << sqrt42<int> << '\n';
    std::cout << ++counter << ". Using stream operator with a constexpr constant as a float: " << sqrt42<float> << '\n';
    std::cout << ++counter << ". Using stream operator with a constexpr constant as a double: " << sqrt42<double> << '\n';
    std::cout << ++counter << ". Using stream operator with a constexpr constant as a long double: " << sqrt42<long double> << '\n';

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

    // Pass the value to a template expression to ensure constant expression
    std::cout << ++counter << ". Using custom function using constexpr Heron's method: " << constexpr_type_v<compute_square_root_heron_method_constexpr(42)> << '\n';

    std::cout << ++counter << ". Using custom function using Newton's method: " << compute_square_root_binary_search_method(42) << '\n';
    std::cout << ++counter << ". Using custom function using Heron's method: " << compute_square_root_heron_method(42) << '\n';
    std::cout << ++counter << ". Using custom function using Bakhshali's method: " << compute_square_root_bakhshali_method(42) << '\n';
    std::cout << ++counter << ". Using infinite digits (only show 1k): " << compute_square_root_digit_by_digit_method(42, 1'000) << '\n';

    // TODO: Add computation time around each computing method

    // TODO: Use std::reduce instead of loops if possible with the help of a structure (overflow, data)
    // TODO: Multi-thread large_integer operations and enable it using CRTP or an executor

    // TODO: Implement different initial estimate methods (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Approximations_that_depend_on_the_floating_point_representation)

    // TODO: Using exponential identity {\displaystyle sqrt(S) = e^(0.5*ln(S) (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Exponential_identity)
    // TODO: Using two-variable iterative method (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#A_two-variable_iterative_method)
    // TODO: Using iterative methods for reciprocal square roots (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Iterative_methods_for_reciprocal_square_roots)
    // TODO: Using Goldschmidt’s algorithm (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Goldschmidt%E2%80%99s_algorithm)
    // TODO: Using Taylor series (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Taylor_series)
    // TODO: Using continued fraction expansion (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Continued_fraction_expansion)

    // TODO: Using quake3 approximation method (not fully portable as double representation specific) (https://www.lomont.org/papers/2003/InvSqrt.pdf)
    // TODO: Using Log base 2 approximation and Newton's method (undefined behavior as union is use to write to one type and read the other) (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Approximations_that_depend_on_the_floating_point_representation)
    // TODO: Using Babylonian approximation method (previous + 2 iterations -> 0.25f*u.x + x/u.x;) (undefined behavior as union is use to write to one type and read the other)
    // TODO: Using Bakhshali approximation (only one iteration)
    // TODO: Using Newton method approximation (not fully portable as double representation specific) (http://www.azillionmonkeys.com/qed/sqroot.html#calcmeth)
    // TODO: Using float biased approximation (not fully portable as float representation specific) (http://bits.stephan-brumme.com/squareRoot.html)

    // TODO: Using recursion instead of loop (possibly using continued fraction expansion implementation)

    // TODO: Implement a digit by digit version that stream the digits
    // TODO: Using an async task
    // TODO: Using a coroutine that return an infinite number of digits
    // TODO: Using a coroutine that use thread (or an executor)
    // TODO: Using a math library? -> not possible on most website
    // TODO: Using a different locale -> maybe not as it's gonna be a ',' instead of a '.' (a little bit boring)
    // TODO: Using constexpr method
    // TODO: Using template parameters, to be computed at compile time
    // TODO: As a reduced expression of prime factor sqrt(2)*sqrt(3)*sqrt(7)

    // I didn't use the following method, even if it's probably the fastest way to compute the square root of a value, as it's not portable
    //double inline __declspec (naked) __fastcall sqrt_asm(double n)
    //{
    //    _asm fld qword ptr[esp + 4]
    //    _asm fsqrt
    //    _asm ret 8
    //}

    return result;
}

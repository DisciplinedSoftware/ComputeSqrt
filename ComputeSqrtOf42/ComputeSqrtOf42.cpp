// Output the square root of 42 in different ways

#include <charconv>
#include <cmath>
#include <concepts>
#include <format>
#include <iostream>
#include <iomanip>
#include <numbers>
#include <ranges>
#include <span>
#include <string>

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

// ----------------------------------------------------------------------------
// Split value into its fractional and exponent part with the exponent always even
// fractional part is (-2, -1], [1, 2)
// return [fractional, exponent]
template<typename T>
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

//template<typename T, typename F>
//auto compute_using

template<typename T>
auto compute_square_root_binary_search_method_fractional(T fractional) -> T {
    assert(1 <= fractional && fractional < 2);

    // Search from zero to fractional
    // Since fractional is between [1,2) the sqrt will be between 0 and sqrt(2)
    T left = 0;
    T right = std::numbers::sqrt2_v<T>;
    T old = std::numeric_limits<T>::max();

    while (true) {
        // Compute the middle point
        const T middle = std::lerp(left, right, T{ 0.5 });

        // No more convergence
        if (middle == old) {
            return middle;
        }

        const T square = middle * middle;

        // Early return optimization as the value was found
        if (square == fractional) {
            return middle;
        }

        // Reduce the interval
        if (square < fractional) {
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
template<typename T>
auto compute_square_root_binary_search_method(T value_) -> T {
    if (!std::isfinite(value_)) {
        return value_;
    }

    // Cannot calculate the root of a negative number
    if (value_ < 0) {
        return NAN;
    }

    // Early return for 0 or 1
    if (value_ * value_ == value_) {
        return value_;
    }

    // Split the problem into the fractional part and the exponent since:
    // sqrt(frac*2^exp)
    // sqrt(frac)*sqrt(2^exp)
    // sqrt(frac)*2^(exp/2)
    // To make this optimization works the exponent must be even
    // This is less optimal when value is a perfect square
    const auto [fractional, exponent] = details::split_into_fractional_and_even_exponent(value_);

    const auto result = details::compute_square_root_binary_search_method_fractional(fractional);

    // Reconstruct the result
    return std::ldexp(result, exponent / 2);
}

// ----------------------------------------------------------------------------
// Overload for integral value
double compute_square_root_binary_search_method(std::integral auto value_)
{
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

// ----------------------------------------------------------------------------
// Compute the square root using heron's method
template<typename T>
auto compute_square_root_heron_method(T value_) -> T {
    if (!std::isfinite(value_)) {
        return value_;
    }

    // Cannot calculate the root of a negative number
    if (value_ < 0) {
        return NAN;
    }

    // Early return for 0 or 1
    if (value_ * value_ == value_) {
        return value_;
    }

    // Split the problem into the fractional part and the exponent since:
    // sqrt(frac*2^exp)
    // sqrt(frac)*sqrt(2^exp)
    // sqrt(frac)*2^(exp/2)
    // To make this optimization works the exponent must be even
    // This is less optimal when value is a perfect square
    const auto [fractional, exponent] = details::split_into_fractional_and_even_exponent(value_);

    T x = fractional / 2;
    T old = x;

    while (true) {
        x = std::lerp(x, fractional / x, 0.5);

        if (x == old) {
            return std::ldexp(x, exponent/2);
        }

        old = x;
    }
}

// ----------------------------------------------------------------------------
// Overload for integral value
double compute_square_root_heron_method(std::integral auto value_)
{
    return compute_square_root_heron_method<double>(value_);
}

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


template<typename T>
auto compute_square_root_bakhshali_method(T value_) -> T {
    if (!std::isfinite(value_)) {
        return value_;
    }

    // Cannot calculate the root of a negative number
    if (value_ < 0) {
        return NAN;
    }

    // Early return for 0 or 1
    if (value_ * value_ == value_) {
        return value_;
    }

    const auto [fractional, exponent] = details::split_into_fractional_and_even_exponent(value_);

    T x = fractional / 2;
    T old = std::numeric_limits<T>::max();

    while (true) {
        const T a = (fractional / x - x) / 2;
        const T d = a - (a * a) / (2 * (x + a));
        const T x_temp = x + d;

        const T abs_epsilon = x_temp * x_temp - value_;

        // Are we closer to our goal
        if (abs_epsilon >= old) {
            return std::ldexp(x_temp, exponent / 2);
        }

        old = abs_epsilon;
        x = x_temp;
    }
}

double compute_square_root_bakhshali_method(std::integral auto value_)
{
    return compute_square_root_bakhshali_method<double>(value_);
}

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


// Replace catch2 with a custom test framework
//
// Implement a version that stream the digits
// For the digits by digits, add rounding for the last digit
// 
// Modify implementations to avoid data copies when signs are different
// This can also be achieved by moving operations to functions that always take the large_integer_using_ref_to_data

class large_integer;
large_integer from_string(const std::string& str_);

class large_integer {
public:
    using underlying_type = std::uint32_t;
    using extended_type = std::uint64_t;
    using signed_extended_type = std::int64_t;

    using collection_type = std::vector<underlying_type>;

    static constexpr auto nb_extended_type_bits = sizeof(underlying_type) * 8;
    static constexpr const extended_type base = extended_type{ 1 } << nb_extended_type_bits;

    large_integer() : large_integer(false, { 0 } ) {}

    large_integer(std::integral auto value_) : large_integer(value_ < 0, to_data_collection(value_)) {}

    large_integer(const char* str_) : large_integer(from_string(str_)) {}
    large_integer(const std::string& str_) : large_integer(from_string(str_)) {}

    large_integer operator-() const {
        return { !sign, data };
    }

    large_integer operator+(const large_integer& other_) const {
        if (sign != other_.sign) {
            return *this - large_integer(!other_.sign, other_.data);
        }

        if (*this < other_) {
            return other_ + *this;
        }

        std::vector<underlying_type> result_data(data.size() + 1, 0);

        size_t index = 0;
        for (; index < other_.data.size(); ++index) {
            const extended_type lhs = data[index];
            const extended_type rhs = other_.data[index];
            const extended_type old_result = result_data[index];
            const extended_type sum = lhs + rhs + old_result;

            // Keep only the lower part that can be stored in underlying_type
            result_data[index] = static_cast<underlying_type>(sum);

            // Compute the overflow that cannot be stored
            result_data[index+1] = sum >> nb_extended_type_bits;
        }

        // Expand the overflow
        for (; index < data.size(); ++index) {
            const extended_type lhs = data[index];
            const extended_type old_result = result_data[index];
            const extended_type sum = lhs + old_result;

            // Keep only the lower part that can be stored in underlying_type
            result_data[index] = static_cast<underlying_type>(sum);

            // Compute the overflow that cannot be stored
            result_data[index + 1] = sum >> nb_extended_type_bits;
        }

        // Trim upper zeros
        index = result_data.size();
        while (index > 1 && result_data[--index] == 0) {
            result_data.pop_back();
        }

        result_data.shrink_to_fit();

        bool result_sign = sign;

        // Handle the -0 case
        if (result_data == collection_type{ 0 }) {
            result_sign = false;
        }

        return { result_sign, result_data };
    }

    large_integer operator-(const large_integer& other_) const {
        if (sign != other_.sign) {
            return *this + large_integer(!other_.sign, other_.data);
        }

        if (*this < other_) {
            return -(other_ - *this);
        }

        assert(data.size() >= other_.data.size());

        std::vector<underlying_type> result_data;
        result_data.reserve(data.size());

        bool carry = false;
        const size_t min_size = other_.data.size();
        size_t index = 0;
        for (; index < min_size; ++index) {
            const signed_extended_type lhs = data[index];
            const signed_extended_type rhs = other_.data[index];

            signed_extended_type value = lhs;
            if (carry) {
                --value;
            }

            if (value < rhs) {
                value += base;
                carry = true;
            }
            else {
                carry = false;
            }

            extended_type diff = value - rhs;
            assert(diff <= std::numeric_limits<underlying_type>::max());
            result_data.emplace_back(static_cast<underlying_type>(diff));
        }

        for (; index < data.size(); ++index) {
            const signed_extended_type lhs = data[index];
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

        // Trim upper zeros
        index = result_data.size();
        while (index > 1 && result_data[--index] == 0) {
            result_data.pop_back();
        }

        result_data.shrink_to_fit();

        bool result_sign = sign;

        // Handle the -0 case
        if (result_data == collection_type{ 0 }) {
            result_sign = false;
        }

        return { result_sign, result_data };
    }

    large_integer operator*(const large_integer& other_) const {
        // Enforce lhs to be larger than rhs
        if (data.size() < other_.data.size()) {
            return other_ * *this;
        }

        bool result_sign = sign != other_.sign;

        collection_type result_data(data.size() * other_.data.size() + 1, 0);

        size_t result_index = 0;
        for (size_t rhs_index = 0; rhs_index < other_.data.size(); ++rhs_index) {
            extended_type overflow{ 0 };
            result_index = rhs_index;
            for (const extended_type lhs : data) {
                const extended_type rhs = other_.data[rhs_index];
                const extended_type old_result = result_data[result_index];

                extended_type value = lhs * rhs + overflow + old_result;
                result_data[result_index] = static_cast<underlying_type>(value);

                overflow = value >> nb_extended_type_bits;

                ++result_index;
            }

            result_data[result_index] = static_cast<underlying_type>(overflow);
        }

        // Trim upper zeros
        size_t index = result_data.size();
        while (index > 1 && result_data[--index] == 0) {
            result_data.pop_back();
        }

        result_data.shrink_to_fit();

        // Handle the -0 case
        if (result_data == collection_type{ 0 }) {
            result_sign = false;
        }

        return { result_sign, result_data };
    }

    std::strong_ordering operator<=>(const large_integer& other_) const {
        if (sign != other_.sign) {
            return sign ? std::strong_ordering::greater : std::strong_ordering::less;
        }

        if (data.size() != other_.data.size()) {
            return (data.size() < other_.data.size()) ? std::strong_ordering::less : std::strong_ordering::greater;
        }

#if __cpp_lib_ranges_zip >= 202110L
        for (const auto& [lhs, rhs] : std::views::reverse(std::views::zip(data, other_.data))) {
            if (lhs != rhs) {
                return (lhs < rhs) ? std::strong_ordering::less : std::strong_ordering::greater;
            }
        }
#else
        auto [it_lhs, it_rhs] = std::ranges::mismatch(data | std::views::reverse, other_.data | std::views::reverse);
        if (it_lhs != data.rend()) {
            return (*it_lhs < *it_rhs) ? std::strong_ordering::less : std::strong_ordering::greater;
        }
#endif

        return std::strong_ordering::equal;
    }

    std::strong_ordering operator<=>(std::integral auto rhs_) const {
        return *this <=> large_integer(rhs_);
    }

    bool operator==(const large_integer& rhs_) const {
        return (*this <=> large_integer(rhs_)) == std::strong_ordering::equal;
    }

    bool operator==(std::integral auto rhs_) const {
        return (*this <=> large_integer(rhs_)) == std::strong_ordering::equal;
    }

private:
    friend std::ostream& operator<<(std::ostream& stream_, const large_integer& value_);
    friend std::istream& operator>>(std::istream& stream_, large_integer& value_);

    constexpr large_integer(bool sign_, std::vector<underlying_type> data_) : sign(sign_), data(std::move(data_)) {}

    static constexpr [[nodiscard]] collection_type to_data_collection(std::integral auto value_)
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
std::string divide_integer_as_string_by_integer(const std::string& number, large_integer::extended_type divisor) {
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

large_integer::extended_type modulo_integer_as_string_by_integer(const std::string& num, large_integer::extended_type a)
{
    // Initialize result
    large_integer::extended_type res = 0;

    // One by one process all digits of 'num'
    for (auto i : num)
        res = (res * 10 + (i - '0')) % a;

    return res;
}

}

std::istream& operator>>(std::istream& stream_, large_integer& value_) {
    if (!stream_) {
        value_ = large_integer();
        return stream_;
    }

    assert(stream_.rdbuf() != nullptr);
    const std::streamsize size = stream_.rdbuf()->in_avail();
    std::string number(size, 0);
    stream_.read(number.data(), size);

    bool sign = false;
    if (!number.empty() && number[0] == '-') {
        sign = true;
        number[0] = '0';
    }

    std::vector<large_integer::underlying_type> data;
    while (number != "0") {
        data.emplace_back(static_cast<large_integer::underlying_type>(details::modulo_integer_as_string_by_integer(number, large_integer::base)));
        number = details::divide_integer_as_string_by_integer(number, large_integer::base);
    }

    assert(!data.empty());
    if (data.empty()) {
        data.emplace_back(0);
    }

    value_ = large_integer(sign, data);

    return stream_;
}

namespace details {

// Function to add two strings representing large integers
std::string add_integers_as_string(const std::string& lhs_, const std::string& rhs_) {
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
std::string multiply_integer_as_string_by_integer(const std::string& num_, large_integer::extended_type factor_) {
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
std::string concatenate_integers_to_string(const std::vector<large_integer::underlying_type>& data_) {
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

std::ostream& operator<<(std::ostream& stream_, const large_integer& value_) {
    assert(!value_.data.empty());

    stream_ << (value_.sign ? "-" : "");
    stream_ << details::concatenate_integers_to_string(value_.data);

    return stream_;
}

[[nodiscard]] large_integer from_string(const std::string& str_) {
    std::istringstream stream(str_);
    large_integer result;
    stream >> result;
    return result;
}

[[nodiscard]] std::string to_string(const large_integer& value_) {
    std::ostringstream stream;
    stream << value_;
    return stream.str();
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



//template<typename T>
//std::string compute_square_root_digit_by_digit_method(T value_, unsigned int max_precision_) {
std::string compute_square_root_digit_by_digit_method(std::integral auto value_, unsigned int max_precision_) {
    //if (!std::isfinite(value_)) {
    //    return std::to_string(value_);
    //}

    // Cannot calculate the root of a negative number
    if (value_ < 0) {
        return std::to_string(NAN);
    }

    if (value_ == 0 || value_ == 1) {
        //const size_t has_dot = (value_ != std::trunc(value_)) && (max_precision_ > 0) ? 1 : 0;
        //const size_t nb_chars = 1 + has_dot + max_precision_;
        //std::string str(nb_chars, '0');
        //auto [ptr, error_code] = std::to_chars(str.data(), str.data() + nb_chars, value_, std::chars_format::fixed, max_precision_);
        //if (error_code != std::errc()) {
        //    std::cout << std::make_error_code(error_code).message() << '\n';
        //}

        //return { str.data(), ptr };

        return std::to_string(value_);
    }

    std::vector<unsigned int> integer_values;

    //T integral{};
    //auto fractional = std::modf(value_, &integral);

    //T residual = integral;
    auto residual = value_;
    //while (residual >= 1) {
    while (residual > 0) {
        //integer_values.emplace_back(static_cast<unsigned int>(std::trunc(std::fmod(residual, T{ 100 }))));
        integer_values.emplace_back(static_cast<unsigned int>(residual % 100));
        residual /= 100;
    }

    //if (integer_values.empty()) {
    //    integer_values.emplace_back(0);
    //}

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

    const auto integral_string_rng =
        std::views::reverse(integer_values) |
        std::views::transform(compute_next_digit) |
        std::views::transform([](auto x) { assert(x < 10); return static_cast<std::string::value_type>(x) + '0'; });

    std::string result_string = std::string(std::begin(integral_string_rng), std::end(integral_string_rng));

    if (/*fractional == 0 &&*/ remainder == 0) {
        return result_string;
    }

    auto precision = max_precision_ > result_string.length() ? max_precision_ - result_string.length() : 0;

    result_string.reserve(result_string.length() + precision + 1);

    result_string += ".";

    constexpr unsigned int next_value = 0;

    while (precision > 0) {
        //fractional *= 100;
        //fractional = std::modf(fractional, &integral);
        //auto next_value = static_cast<unsigned int>(std::trunc(integral))

        result_string += static_cast<std::string::value_type>(compute_next_digit(next_value)) + '0';

        if (remainder == 0) {
            break;
        }

        --precision;
    }

    //auto compute_next_digit_debug = [&remainder, &result](auto current_) {
    //        //std::cout << "remainder" << '\n' << remainder << '\n';
    //        //std::cout << "result" << '\n' << result << '\n';

    //        // find x * (20p + x) <= remainder*100+current
    //        large_integer current_remainder = remainder * 100 + current_;

    //        //std::cout << "remainder * 100 + current_" << '\n' << current_remainder << '\n';

    //        unsigned int x{ 0 };
    //        large_integer sum{ 0 };
    //        auto expanded_result = result * 20;

    //        //std::cout << "result * 20" << '\n' << expanded_result << '\n';

    //        while (true) {
    //            ++x;

    //            large_integer next_sum{ 0 };
    //            next_sum = (expanded_result + x);

    //            //std::cout << "expanded_result + x" << '\n' << next_sum << '\n';

    //            next_sum = next_sum * x;

    //            //std::cout << "next_sum * x" << '\n' << next_sum << '\n';

    //            if (next_sum > current_remainder) {
    //                --x;
    //                break;
    //            }

    //            sum = next_sum;
    //        }

    //        assert(x < 10);

    //        result = result * 10 + x;
    //        remainder = current_remainder - sum;

    //        std::cout << "current_remainder" << '\n' << current_remainder << '\n';
    //        std::cout << "sum" << '\n' << sum << '\n';
    //        std::cout << "current_remainder - sum" << '\n' << remainder << '\n';
    //        std::cout << "result * 10 + x" << '\n' << result << '\n';

    //        return x;
    //    };

    // Compute one more digit to round the last one
    const auto rounding_digit = compute_next_digit(next_value);

    const double last_digit = result_string.back() - '0';
    const double rounded_last_digit = std::round(last_digit + (static_cast<double>(rounding_digit) / 10.0));

    result_string.back() = static_cast<std::string::value_type>(rounded_last_digit) + '0';

    return result_string;
}

//std::string compute_square_root_digit_by_digit_method(std::integral auto value_, unsigned int max_precision_) {
//    return compute_square_root_digit_by_digit_method<double>(value_, max_precision_);
//}

TEST_CASE("compute_square_root_digit_by_digit_method") {
    using namespace std::string_literals;
    CHECK(compute_square_root_digit_by_digit_method(-1, 2) == "nan"s);
    CHECK(compute_square_root_digit_by_digit_method(0, 0) == "0"s);
    CHECK(compute_square_root_digit_by_digit_method(-0, 0) == "0"s);
    CHECK(compute_square_root_digit_by_digit_method(1, 0) == "1"s);
    CHECK(compute_square_root_digit_by_digit_method(4, 0) == "2"s);
    //CHECK(compute_square_root_digit_by_digit_method(2, 31) == "1.4142135623730950488016887242097"s);
    //CHECK(compute_square_root_digit_by_digit_method(780.14, 30) == "27.930986377140353329545077471771"s);
    //CHECK(compute_square_root_digit_by_digit_method(0.5, 32) == "0.70710678118654752440084436210485"s);

    CHECK(compute_square_root_digit_by_digit_method(42, 31) == "6.480740698407860230965967436088"s);
    CHECK(compute_square_root_digit_by_digit_method(42, 100) == "6.480740698407860230965967436087996657705204307058346549711354397809617377844044371400360906605610236"s);

    //CHECK(compute_square_root_digit_by_digit_method(1e-15, 38) == "0.00000031622776601683793319988935444327"s);
}


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

    std::cout << ++counter << ". Using std::sqrt(float): " << std::sqrt(42.0f) << '\n';
    std::cout << ++counter << ". Using std::sqrt(double): " << std::sqrt(42.0) << '\n';
    std::cout << ++counter << ". Using std::sqrt(long double): " << std::sqrt(42.0l) << '\n';
    std::cout << ++counter << ". Using std::sqrtf(float): " << std::sqrt(42.0f) << '\n';
    std::cout << ++counter << ". Using std::sqrtl(long double): " << std::sqrt(42.0l) << '\n';

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

    std::cout << ++counter << ". Using custom function using Newton's method: " << compute_square_root_binary_search_method(42) << '\n';
    std::cout << ++counter << ". Using custom function using Heron's method: " << compute_square_root_heron_method(42) << '\n';
    std::cout << ++counter << ". Using custom function using Bakhshali's method: " << compute_square_root_bakhshali_method(42) << '\n';
    std::cout << ++counter << ". Using infinite digits (only show 1k): " << compute_square_root_digit_by_digit_method(42, 1'000) << '\n';

    // Using my own sqrt function
    // Using recursion and using loop
    // Using an async task
    // Using coroutine
    // Using a coroutine that return an infinite number of digits
    // Using a coroutine that use thread
    // Using a math library?
    // Using a different local
    // Using template parameters
    // As a reduced expression of prime factor sqrt(2)*sqrt(3)*sqrt(7)
    // Using 42^(1/2) with std::pow

    return result;
}

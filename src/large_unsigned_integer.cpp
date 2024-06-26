#include "large_unsigned_integer.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>

#include "utility.hpp"

namespace {

using collection_type = large_unsigned_integer::collection_type;
using underlying_type = large_unsigned_integer::underlying_type;
using extended_type = large_unsigned_integer::extended_type;
using signed_extended_type = large_unsigned_integer::signed_extended_type;

constexpr const auto nb_extended_type_bits = large_unsigned_integer::nb_extended_type_bits;
constexpr const auto base = large_unsigned_integer::base;

// ------------------------------------------------------------------------
// Helper function that compare 2 large unsigned intergers
[[nodiscard]] std::strong_ordering compare_large_unsigned_integer(const collection_type& lhs_, const collection_type& rhs_) {
    if (lhs_.size() != rhs_.size()) {
        return (lhs_.size() < rhs_.size())
            ? std::strong_ordering::less
            : std::strong_ordering::greater;
    }

#if defined(__cpp_lib_ranges_zip) && __cpp_lib_ranges_zip >= 202110L
    for (const auto& [lhs, rhs] :
        std::views::zip(lhs_, rhs_) | std::views::reverse) {
        if (lhs != rhs) {
            return (lhs < rhs)
                ? std::strong_ordering::less
                : std::strong_ordering::greater;
        }
    }
#else
    auto [it_lhs, it_rhs] = std::ranges::mismatch(lhs_ | std::views::reverse,
        rhs_ | std::views::reverse);
    if (it_lhs != lhs_.rend()) {
        return (*it_lhs < *it_rhs)
            ? std::strong_ordering::less
            : std::strong_ordering::greater;
    }
#endif

    return std::strong_ordering::equal;
}

// ------------------------------------------------------------------------
// Validate is collections are sorted (lhs_ <= rhs_)
[[nodiscard]] bool sorted(const collection_type& lhs_, const collection_type& rhs_) {
    const auto result = compare_large_unsigned_integer(lhs_, rhs_);
    return result == std::strong_ordering::equal || result == std::strong_ordering::greater;
}

// ------------------------------------------------------------------------
// Helper function that trims the usless upper zeros
[[nodiscard]] collection_type trim_upper_zeros(collection_type&& data_) {
    auto it = std::ranges::find_if(data_ | std::views::reverse, [](auto value_) { return value_ != 0; });
    data_.erase(it.base(), data_.end());
    return data_;
}

// ------------------------------------------------------------------------
// cleanup data to reduce the memory footprint and useless computation
[[nodiscard]] collection_type cleanup(collection_type&& data_) {
    // Remove useless zeros
    data_ = trim_upper_zeros(std::move(data_));

    // Free unused data
    data_.shrink_to_fit();

    return data_;
}

// ------------------------------------------------------------------------
// Helper function that add 2 sorted large unsigned intergers
[[nodiscard]] collection_type add_large_unsigned_integer_sorted(const collection_type& lhs_, const collection_type& rhs_) {
    assert(sorted(lhs_, rhs_));

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

    return cleanup(std::move(result_data));
}

// ------------------------------------------------------------------------

std::tuple<signed_extended_type, bool> subtract_one_digit(signed_extended_type lhs_value_, signed_extended_type rhs_value_, bool carry_) {
    if (carry_) {
        --lhs_value_;
    }

    if (lhs_value_ < rhs_value_) {
        lhs_value_ += base;
        carry_ = true;
    } else {
        carry_ = false;
    }

    return { lhs_value_ - rhs_value_, carry_ };
};

// ------------------------------------------------------------------------
// Helper function that subtract 2 sorted large unsigned intergers
[[nodiscard]] collection_type subtract_large_unsigned_integer_sorted(const collection_type& lhs_, const collection_type& rhs_) {
    assert(sorted(lhs_, rhs_));

    std::vector<underlying_type> result_data;
    result_data.reserve(lhs_.size());

    // Subtract every digit of rhs from the corresponding lhs
    bool carry = false;
    const size_t min_size = rhs_.size();
    size_t index = 0;
    std::generate_n(std::back_inserter(result_data), min_size, [&carry, &index, &lhs_, &rhs_] {
        const signed_extended_type lhs_value = lhs_[index];
        const signed_extended_type rhs_value = rhs_[index];
        ++index;

        const auto [difference, new_carry] = subtract_one_digit(lhs_value, rhs_value, carry);
        carry = new_carry;

        assert(difference >= 0);
        assert(difference <= std::numeric_limits<underlying_type>::max());
        return static_cast<underlying_type>(difference);
    });

    // Extend the carry to the rest of lhs
    for (; index < lhs_.size(); ++index) {
        const signed_extended_type lhs_value = lhs_[index];
        const signed_extended_type rhs_value = 0;

        const auto [difference, new_carry] = subtract_one_digit(lhs_value, rhs_value, carry);
        carry = new_carry;

        assert(difference >= 0);
        assert(difference <= std::numeric_limits<underlying_type>::max());
        result_data.emplace_back(static_cast<underlying_type>(difference));
    }

    return cleanup(std::move(result_data));
}

// ------------------------------------------------------------------------
// Helper function that multiply 2 sorted large unsigned intergers
[[nodiscard]] collection_type multiply_large_unsigned_integer_sorted(const collection_type& lhs_, const collection_type& rhs_) {
    assert(sorted(lhs_, rhs_));

    collection_type result_data(lhs_.size() * rhs_.size() + 1, 0);

    // Multiply each digit of rhs with each digit of lhs
    size_t result_index = 0;
    for (size_t rhs_index = 0; rhs_index < rhs_.size(); ++rhs_index) {
        extended_type overflow{ 0 };
        result_index = rhs_index;
        for (const extended_type lhs_value : lhs_) {
            const extended_type rhs_value = rhs_[rhs_index];
            const extended_type old_result = result_data[result_index];

            const extended_type value = lhs_value * rhs_value + overflow + old_result;
            result_data[result_index] = static_cast<underlying_type>(value);

            overflow = value >> nb_extended_type_bits;

            ++result_index;
        }

        result_data[result_index] = static_cast<underlying_type>(overflow);
    }

    return cleanup(std::move(result_data));
}

} // Anonymous namespace

// ----------------------------------------------------------------------------

large_unsigned_integer::large_unsigned_integer() : large_unsigned_integer(0u) {}

// ----------------------------------------------------------------------------

large_unsigned_integer::large_unsigned_integer(std::vector<underlying_type> data_)
    : data(cleanup(std::move(data_))) {}

// ----------------------------------------------------------------------------

[[nodiscard]] large_unsigned_integer large_unsigned_integer::operator+(const large_unsigned_integer& other_) const {
    // Enforce lhs to be larger than rhs
    if (*this < other_) {
        return other_ + (*this);
    }

    return add_large_unsigned_integer_sorted(data, other_.data);
}

// ----------------------------------------------------------------------------

[[nodiscard]] large_unsigned_integer large_unsigned_integer::operator-(const large_unsigned_integer& other_) const {
    assert(*this >= other_);    // Enforce a positive result

    if (*this < other_) {
        return {};
    }

    return subtract_large_unsigned_integer_sorted(data, other_.data);
}

// ----------------------------------------------------------------------------

[[nodiscard]] large_unsigned_integer large_unsigned_integer::operator*(const large_unsigned_integer& other_) const {
    // Enforce lhs to be larger than rhs
    if (*this < other_) {
        return other_ * (*this);
    }

    return multiply_large_unsigned_integer_sorted(data, other_.data);
}

// ----------------------------------------------------------------------------

[[nodiscard]] std::strong_ordering large_unsigned_integer::operator<=>(const large_unsigned_integer& other_) const {
    return compare_large_unsigned_integer(data, other_.data);
}

// ------------------------------------------------------------------------

[[nodiscard]] bool large_unsigned_integer::operator==(const large_unsigned_integer& other_) const {
    return this->data == other_.data;
}

// ------------------------------------------------------------------------

[[nodiscard]] const large_unsigned_integer::collection_type& large_unsigned_integer::get_data() const {
    return data;
}

// ----------------------------------------------------------------------------

namespace details {

using extended_type = large_unsigned_integer::extended_type;

// ----------------------------------------------------------------------------

// Perform division of large number represented as a string
[[nodiscard]] std::string divide_integer_as_string_by_integer(const std::string& number_, extended_type divisor_) {
    assert(!number_.empty());

    // Find prefix of number that is larger than divisor.
    extended_type sum{ 0 };
    auto it = std::ranges::find_if(number_, [&sum, divisor_](char c) {
        sum = sum * 10 + (c - '0');
        return sum >= divisor_;
    });

    const auto next_char = to_char(sum / divisor_);
    std::string result({ next_char });
    result.reserve(number_.size());

    if (it == std::end(number_)) {
        return result;
    }

    std::ranges::transform(++it, std::end(number_), std::back_inserter(result), [&sum, divisor_](char c) {
        sum = (sum % divisor_) * 10 + to_value(c);
        const auto next_char = to_char(sum / divisor_);
        return next_char;
    });

    return result;
}

// ----------------------------------------------------------------------------

[[nodiscard]] extended_type modulo_integer_as_string_by_integer(const std::string& number_, extended_type divisor_) {
    extended_type result = 0;

    // Sequencially apply modulo
    for (const auto digit : number_) {
        result = (result * 10 + to_value(digit)) % divisor_;
    }

    return result;
}

// ----------------------------------------------------------------------------

[[nodiscard]] bool is_number_well_formed(const std::string& str_) {
    unsigned int dot_counter = 0;

    return std::ranges::all_of(str_, [&dot_counter](auto digit) {
        // This assume a locale that split number using '.'
        if (digit == '.') {
            ++dot_counter;
            return dot_counter == 1;    // Must have only one '.'
        }

        return std::isdigit(digit) != 0;
    });
}

} // namespace details

// ----------------------------------------------------------------------------

[[nodiscard]] std::optional<large_unsigned_integer> large_unsigned_integer::from_string(const std::string& str_) {
    assert(!str_.empty());
    if (str_.empty()) {
        return {};
    }

    const bool well_formed = details::is_number_well_formed(str_);

    assert(well_formed);
    if (!well_formed) {
        return {};
    }

    std::string number = str_;

    std::vector<large_unsigned_integer::underlying_type> data;
    do {
        data.emplace_back(static_cast<large_unsigned_integer::underlying_type>(
            details::modulo_integer_as_string_by_integer(number,
                large_unsigned_integer::base)));
        number = details::divide_integer_as_string_by_integer(number,
            large_unsigned_integer::base);
    } while (number != "0");

    return std::make_optional<large_unsigned_integer>(data);
}

// ----------------------------------------------------------------------------

namespace details {

[[nodiscard]] inline std::tuple<std::string, extended_type> add_integers(std::string&& sum_, extended_type lhs_, extended_type rhs_, extended_type carry_) {
    const auto digitSum = lhs_ + rhs_ + carry_;
    carry_ = digitSum / 10;

    // String is constructed in reverse
    sum_ += to_char(digitSum % 10);
    return { sum_, carry_ };
}

// ----------------------------------------------------------------------------

// Function to add two strings representing large integers
[[nodiscard]] std::string add_integers_as_string(const std::string& lhs_, const std::string& rhs_) {
    assert(!lhs_.empty() && !rhs_.empty());

    if (lhs_.size() < rhs_.size()) {
        return add_integers_as_string(rhs_, lhs_);
    }

    extended_type carry = 0;
    std::string sum;
    sum.reserve(lhs_.size() + 1);

    // Add rhs digits
#if defined(__cpp_lib_ranges_zip) && __cpp_lib_ranges_zip >= 202110L
    for (const auto [lhs_digit, rhs_digit] : std::views::zip(lhs_, rhs_)) {
#else
    for (size_t index = 0; index < rhs_.size(); ++index) {
        const auto lhs_digit = to_value(lhs_[index]);
        const auto rhs_digit = to_value(rhs_[index]);
#endif
        std::tie(sum, carry) = add_integers(std::move(sum), lhs_digit, rhs_digit, carry);
}

    // Propagate carry to lhs
    for (const auto lhs_char : lhs_ | std::views::drop(rhs_.size())) {
        const auto lhs_digit = to_value(lhs_char);
        constexpr const auto rhs_digit = 0;

        std::tie(sum, carry) = add_integers(std::move(sum), lhs_digit, rhs_digit, carry);
    }

    // Propagate carry
    if (carry > 0) {
        constexpr const auto lhs_digit = 0;
        constexpr const auto rhs_digit = 0;
        std::tie(sum, carry) = add_integers(std::move(sum), lhs_digit, rhs_digit, carry);
    }

    assert(carry == 0);

    auto reversed = std::views::reverse(sum);
    return { std::begin(reversed), std::end(reversed) };
}

// ----------------------------------------------------------------------------

// Function to multiply a string representing a large integer by an integer
[[nodiscard]] std::string multiply_integer_as_string_by_integer(const std::string & num_, large_unsigned_integer::extended_type factor_) {
    std::string result;
    large_unsigned_integer::extended_type carry = 0;
    for (auto digit_str : std::views::reverse(num_)) {
        large_unsigned_integer::extended_type digit = digit_str - '0';
        large_unsigned_integer::extended_type product = digit * factor_ + carry;
        carry = product / 10;
        result = std::to_string(product % 10) + result;
    }

    if (carry > 0) {
        result = std::to_string(carry) + result;
    }

    return result;
}

// ----------------------------------------------------------------------------

// Recompose data, where values are represented as data[i]*base^i, into a base 10 string
[[nodiscard]] std::string recompose_data_as_base_10_string(const large_unsigned_integer::collection_type & data_, large_unsigned_integer::extended_type base_) {
    std::string result = "0";
    unsigned int power = 0;
    for (auto single_data : data_) {
        // Multiply the current element by the large_unsigned_integer base
        std::string temp = std::to_string(single_data);
        for (size_t j = 0; j < power; ++j) {
            temp = multiply_integer_as_string_by_integer(temp, base_);
        }
        ++power;
        // Add the current number to the result
        result = add_integers_as_string(result, temp);
    }
    return result;
}

    } // namespace details

    // ----------------------------------------------------------------------------

[[nodiscard]] std::string to_string(const large_unsigned_integer& value_) {
    return details::recompose_data_as_base_10_string(value_.get_data(), large_unsigned_integer::base);
}

// ----------------------------------------------------------------------------

std::istream& operator>>(std::istream& stream_, large_unsigned_integer& value_) {
    // Assume that the rdbuf exist and that the number is fully contains in the
    // buffer
    assert(stream_.rdbuf() != nullptr);
    const std::streamsize size = stream_.rdbuf()->in_avail();
    std::string number(size, 0);
    stream_.read(number.data(), size);

    auto result = large_unsigned_integer::from_string(number);
    assert(result.has_value());
    value_ = std::move(result).value_or(0u);

    return stream_;
}

// ----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& stream_, const large_unsigned_integer& value_) {
    stream_ << to_string(value_);
    return stream_;
}
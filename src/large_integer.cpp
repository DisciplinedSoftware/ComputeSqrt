#include "large_integer.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>

namespace {

using collection_type = large_integer::collection_type;
using underlying_type = large_integer::underlying_type;
using extended_type = large_integer::extended_type;
using signed_extended_type = large_integer::signed_extended_type;

constexpr const auto nb_extended_type_bits = large_integer::nb_extended_type_bits;
constexpr const auto base = large_integer::base;

// ------------------------------------------------------------------------
// Helper function that compare 2 large unsigned intergers
[[nodiscard]] std::strong_ordering compare_large_unsigned_integer(const collection_type& lhs_, const collection_type& rhs_) {
    if (lhs_.size() != rhs_.size()) {
        return (lhs_.size() < rhs_.size())
            ? std::strong_ordering::less
            : std::strong_ordering::greater;
    }

#if __cpp_lib_ranges_zip >= 202110L
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
    size_t index = data_.size();
    while (index > 1 && data_[--index] == 0) {
        data_.pop_back();
    }

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
// Helper function that subtract 2 sorted large unsigned intergers
[[nodiscard]] collection_type subtract_large_unsigned_integer_sorted(const collection_type& lhs_, const collection_type& rhs_) {
    assert(sorted(lhs_, rhs_));

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
        } else {
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
        } else {
            carry = false;
        }

        assert(value <= std::numeric_limits<underlying_type>::max());
        result_data.emplace_back(static_cast<underlying_type>(value));
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

            extended_type value = lhs_value * rhs_value + overflow + old_result;
            result_data[result_index] = static_cast<underlying_type>(value);

            overflow = value >> nb_extended_type_bits;

            ++result_index;
        }

        result_data[result_index] = static_cast<underlying_type>(overflow);
    }

    return cleanup(std::move(result_data));
}

} // Anonymous namespace

// ------------------------------------------------------------------------
// Helper class that avoid data copies
class large_integer::data_ref {
public:
    // Large integer constructor
    data_ref(const large_integer& other_);

    // Raw data constructor
    data_ref(bool sign_, const std::vector<underlying_type>& data_);

    // Operators
    [[nodiscard]] large_integer operator+(const data_ref& other_) const;
    [[nodiscard]] large_integer operator-(const data_ref& other_) const;
    [[nodiscard]] large_integer operator*(const data_ref& other_) const;
    [[nodiscard]] std::strong_ordering operator<=>(const data_ref& other_) const;

private:
    bool sign{};
    std::reference_wrapper<const collection_type> data;
};

// ----------------------------------------------------------------------------

large_integer::data_ref::data_ref(const large_integer& other_)
    : data_ref(other_.sign, std::cref(other_.data)) {}

// ----------------------------------------------------------------------------

large_integer::data_ref::data_ref(bool sign_, const std::vector<underlying_type>& data_)
    : sign(sign_), data(std::cref(data_)) {}

// --------------------------------------------------------------------

[[nodiscard]] large_integer large_integer::data_ref::operator+(const data_ref& other_) const {
    // Enforce signs to be the same
    if (sign != other_.sign) {
        return *this - data_ref(!other_.sign, other_.data.get());
    }

    // Enforce lhs to be larger than rhs
    if (*this < other_) {
        return other_ + *this;
    }

    bool result_sign = sign;
    auto result_data = add_large_unsigned_integer_sorted(data.get(), other_.data.get());

    return { result_sign, result_data };
}

// ----------------------------------------------------------------------------

[[nodiscard]] large_integer large_integer::data_ref::operator-(const data_ref& other_) const {
    // Enforce signs to be the same
    if (sign != other_.sign) {
        return *this + data_ref(!other_.sign, other_.data.get());
    }

    // Enforce lhs to be larger than rhs
    if (*this < other_) {
        auto result = other_ - *this;
        return { !result.sign, result.data };
    }

    bool result_sign = sign;
    auto result_data = subtract_large_unsigned_integer_sorted(data.get(), other_.data.get());

    return { result_sign, result_data };
}

// ----------------------------------------------------------------------------

[[nodiscard]] large_integer large_integer::data_ref::operator*(const data_ref& other_) const {
    // Enforce signs to be the same
    if (sign != other_.sign) {
        bool result_sign = true;
        auto result = *this * data_ref(!other_.sign, other_.data.get());
        return { result_sign, result.data };
    }

    // Enforce lhs to be larger than rhs
    if (*this < other_) {
        return other_ * *this;
    }

    bool result_sign = false;
    auto result_data = multiply_large_unsigned_integer_sorted(data.get(), other_.data.get());

    return { result_sign, result_data };
}

// ----------------------------------------------------------------------------

[[nodiscard]] std::strong_ordering large_integer::data_ref::operator<=>(const data_ref& other_) const {
    if (sign != other_.sign) {
        return sign
            ? std::strong_ordering::greater
            : std::strong_ordering::less;
    }

    return compare_large_unsigned_integer(data, other_.data);
}

// ----------------------------------------------------------------------------

large_integer::large_integer() : large_integer(false, { 0 }) {}

// ----------------------------------------------------------------------------

large_integer::large_integer(bool sign_, std::vector<underlying_type> data_)
    : sign(sign_), data(cleanup(std::move(data_))) {
    fix_minus_zeros();
}

// ------------------------------------------------------------------------

[[nodiscard]] large_integer large_integer::operator-() const {
    return { !sign, data };
}

// ------------------------------------------------------------------------

[[nodiscard]] large_integer large_integer::operator+(const large_integer& other_) const {
    return data_ref(*this) + data_ref(other_);
}

// ------------------------------------------------------------------------

[[nodiscard]] large_integer large_integer::operator-(const large_integer& other_) const {
    return data_ref(*this) - data_ref(other_);
}

// ------------------------------------------------------------------------

[[nodiscard]] large_integer large_integer::operator*(const large_integer& other_) const {
    return data_ref(*this) * data_ref(other_);
}

// ----------------------------------------------------------------------------

[[nodiscard]] std::strong_ordering large_integer::operator<=>(const large_integer& other_) const {
    return data_ref(*this) <=> data_ref(other_);
}

// ------------------------------------------------------------------------

[[nodiscard]] bool large_integer::operator==(const large_integer& rhs_) const {
    return (*this <=> rhs_) == std::strong_ordering::equal;
}

// ------------------------------------------------------------------------

[[nodiscard]] bool large_integer::get_sign() const {
    return sign;
}

// ------------------------------------------------------------------------

[[nodiscard]] const large_integer::collection_type& large_integer::get_data() const {
    return data;
}

// ----------------------------------------------------------------------------

void large_integer::fix_minus_zeros() {
    if (data == collection_type{ 0 }) {
        sign = false;
    }
}

// ----------------------------------------------------------------------------

namespace details {

// A function to perform division of large numbers
[[nodiscard]] std::string divide_integer_as_string_by_integer(const std::string& number_, large_integer::extended_type divisor_) {
    // As result can be very large store it in string
    std::string ans;

    // Find prefix of number that is larger than divisor.
    size_t idx = 0;
    large_integer::extended_type temp = number_[idx] - '0';
    while (idx < (number_.size() - 1) && temp < divisor_) {
        temp = temp * 10 + (number_[++idx] - '0');
    }

    // Repeatedly divide divisor with temp. After
    // every division, update temp to include one
    // more digit.
    while ((number_.size() - 1) > idx) {
        // Store result in answer i.e. temp / divisor
        ans += static_cast<char>(temp / divisor_) + '0';

        // Take next digit of number_
        temp = (temp % divisor_) * 10 + number_[++idx] - '0';
    }

    ans += static_cast<char>(temp / divisor_) + '0';

    // else return ans
    return ans;
}

// ----------------------------------------------------------------------------

[[nodiscard]] large_integer::extended_type modulo_integer_as_string_by_integer(const std::string& number_, large_integer::extended_type divisor_) {
    large_integer::extended_type res = 0;

    // One by one process all digits of 'number_'
    for (const auto digit : number_) {
        res = (res * 10 + (digit - '0')) % divisor_;
    }

    return res;
}

} // namespace details

// ----------------------------------------------------------------------------

[[nodiscard]] std::optional<large_integer> large_integer::from_string(const std::string& str_) {
    const bool is_string_empty = str_.empty();
    assert(!is_string_empty);
    if (is_string_empty) {
        return {};
    }

    bool sign = false;
    auto begin = std::begin(str_);
    if (*begin == '-') {
        sign = true;
        ++begin;
    }

    const bool is_number_well_formed = std::all_of(begin, std::end(str_), [](auto digit) {
        if (std::isdigit(digit)) {
            return true;
        }

        // This assume a locale that split number using '.'
        if (digit == '.') {
            static unsigned int dot_counter = 0;
            ++dot_counter;
            assert(dot_counter == 1);
            return true;
        }

        return false; });

    assert(is_number_well_formed);
    if (!is_number_well_formed) {
        return {};
    }

    std::string number(begin, std::end(str_));

    std::vector<large_integer::underlying_type> data;
    do {
        data.emplace_back(static_cast<large_integer::underlying_type>(
            details::modulo_integer_as_string_by_integer(number,
                large_integer::base)));
        number = details::divide_integer_as_string_by_integer(number,
            large_integer::base);
    } while (number != "0");

    return std::make_optional<large_integer>(sign, data);
}

// ----------------------------------------------------------------------------

namespace details {

// Function to add two strings representing large integers
[[nodiscard]] std::string add_integers_as_string(const std::string& lhs_, const std::string& rhs_) {
    assert(!lhs_.empty() && !rhs_.empty());

    std::string sum;
    int carry = 0;
    auto lhs_iter = lhs_.rbegin();
    auto rhs_iter = rhs_.rbegin();

    while (lhs_iter != lhs_.rend() || rhs_iter != rhs_.rend() || carry > 0) {
        int lhs_digit = 0;
        if (lhs_iter != lhs_.rend()) {
            lhs_digit = *lhs_iter - '0';
            ++lhs_iter;
        }

        int rhs_digit = 0;
        if (rhs_iter != rhs_.rend()) {
            rhs_digit = *rhs_iter - '0';
            ++rhs_iter;
        }

        const int digitSum = lhs_digit + rhs_digit + carry;
        carry = digitSum / 10;
        sum = std::to_string(digitSum % 10) + sum;
    }

    return sum;
}

// ----------------------------------------------------------------------------

// Function to multiply a string representing a large integer by an integer
[[nodiscard]] std::string multiply_integer_as_string_by_integer(const std::string& num_, large_integer::extended_type factor_) {
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

// ----------------------------------------------------------------------------

// Recompose data, where values are represented as data[i]*base^i, into a base 10 string
[[nodiscard]] std::string recompose_data_as_base_10_string(const large_integer::collection_type& data_, large_integer::extended_type base_) {
    std::string result = "0";
    unsigned int power = 0;
    for (auto single_data : data_) {
        // Multiply the current element by the large_integer base
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

[[nodiscard]] std::string to_string(const large_integer& value_) {
    auto result = details::recompose_data_as_base_10_string(value_.get_data(), large_integer::base);

    if (value_.get_sign()) {
        return '-' + result;
    }

    return result;
}

// ----------------------------------------------------------------------------

std::istream& operator>>(std::istream& stream_, large_integer& value_) {
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

// ----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& stream_, const large_integer& value_) {
    stream_ << to_string(value_);
    return stream_;
}
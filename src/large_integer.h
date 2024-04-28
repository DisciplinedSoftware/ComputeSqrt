
#ifndef LARGE_INTEGER
#define LARGE_INTEGER

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <ranges>
#include <string>
#include <vector>


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

    [[nodiscard]] static constexpr std::optional<large_integer> from_string(const std::string& str_);

    constexpr large_integer() : large_integer(false, { 0 }) {}

    constexpr large_integer(std::integral auto value_)
        : large_integer(value_ < 0, to_data_collection(value_)) {}

    constexpr large_integer(bool sign_, std::vector<underlying_type> data_)
        : sign(sign_), data(std::move(data_)) {}

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

    [[nodiscard]] constexpr auto operator<=>(const large_integer& other_) const {
        return large_integer_data_ref(*this) <=> large_integer_data_ref(other_);
    }

    [[nodiscard]] constexpr auto operator<=>(std::integral auto rhs_) const {
        return *this <=> large_integer(rhs_);
    }

    [[nodiscard]] constexpr bool operator==(const large_integer& rhs_) const {
        return (*this <=> large_integer(rhs_)) == std::strong_ordering::equal;
    }

    [[nodiscard]] constexpr bool operator==(std::integral auto rhs_) const {
        return (*this <=> large_integer(rhs_)) == std::strong_ordering::equal;
    }

    [[nodiscard]] constexpr bool get_sign() const {
        return sign;
    }

    [[nodiscard]] constexpr const collection_type& get_data() const {
        return data;
    }

private:
    class large_integer_data_ref {
    public:
        constexpr large_integer_data_ref(const large_integer& other_)
            : large_integer_data_ref(other_.sign, std::cref(other_.data)) {}

        constexpr large_integer_data_ref(bool sign_, const std::vector<underlying_type>& data_)
            : sign(sign_), data(std::cref(data_)) {}

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
                return sign
                    ? std::strong_ordering::greater
                    : std::strong_ordering::less;
            }

            return compare_large_unsigned_integer(data, other_.data);
        }

    private:
        bool sign{};
        std::reference_wrapper<const collection_type> data;
    };

    [[nodiscard]] static constexpr collection_type to_data_collection(std::integral auto value_) {
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

    [[nodiscard]] static constexpr bool sorted(const collection_type& lhs_, const collection_type& rhs_) {
        const auto result = compare_large_unsigned_integer(lhs_, rhs_);
        return result == std::strong_ordering::equal || result == std::strong_ordering::greater;
    }

    [[nodiscard]] static constexpr collection_type add_large_unsigned_integer_sorted(const collection_type& lhs_, const collection_type& rhs_) {
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

        return result_data;
    }

    [[nodiscard]] static constexpr collection_type subtract_large_unsigned_integer_sorted(const collection_type& lhs_, const collection_type& rhs_) {
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

        return result_data;
    }

    [[nodiscard]] static constexpr collection_type multiply_large_unsigned_integer_sorted(const collection_type& lhs_, const collection_type& rhs_) {
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

        return result_data;
    }

    [[nodiscard]] static constexpr std::strong_ordering compare_large_unsigned_integer(const collection_type& lhs_, const collection_type& rhs_) {
        if (lhs_.size() != rhs_.size()) {
            return (lhs_.size() < rhs_.size())
                ? std::strong_ordering::less
                : std::strong_ordering::greater;
        }

#if __cpp_lib_ranges_zip >= 202110L
        for (const auto& [lhs, rhs] :
            std::views::zip(lhs_, rhs_) | std::views::reverse) {
            if (lhs != rhs) {
                return (lhs < rhs) ? std::strong_ordering::less
                    : std::strong_ordering::greater;
            }
        }
#else
        auto [it_lhs, it_rhs] = std::ranges::mismatch(lhs_ | std::views::reverse,
            rhs_ | std::views::reverse);
        if (it_lhs != lhs_.rend()) {
            return (*it_lhs < *it_rhs) ? std::strong_ordering::less
                : std::strong_ordering::greater;
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
[[nodiscard]] constexpr std::string divide_integer_as_string_by_integer(const std::string& number_, large_integer::extended_type divisor_) {
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

[[nodiscard]] constexpr large_integer::extended_type modulo_integer_as_string_by_integer(const std::string& number_, large_integer::extended_type divisor_) {
    large_integer::extended_type res = 0;

    // One by one process all digits of 'number_'
    for (const auto digit : number_) {
        res = (res * 10 + (digit - '0')) % divisor_;
    }

    return res;
}

} // namespace details

[[nodiscard]] constexpr std::optional<large_integer> large_integer::from_string(const std::string& str_) {
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

    return std::make_optional<large_integer>(sign, std::move(data));
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

} // namespace details

[[nodiscard]] constexpr std::string to_string(const large_integer& value_) {
    auto result = details::concatenate_integers_to_string(value_.get_data());

    if (value_.get_sign()) {
        return '-' + result;
    }

    return result;
}

std::istream& operator>>(std::istream& stream_, large_integer& value_);
std::ostream& operator<<(std::ostream& stream_, const large_integer& value_);

#endif // LARGE_INTEGER
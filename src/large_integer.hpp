#ifndef LARGE_INTEGER_HPP
#define LARGE_INTEGER_HPP

#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <vector>


// ----------------------------------------------------------------------------
// Large integer to handle infinitely large integer number
class large_integer {
public:
    using underlying_type = std::uint32_t;
    using extended_type = std::uint64_t;
    using signed_extended_type = std::int64_t;

    using collection_type = std::vector<underlying_type>;

    static constexpr const auto nb_extended_type_bits = sizeof(underlying_type) * 8;
    static constexpr const extended_type base = extended_type{ 1 } << nb_extended_type_bits;

    // Factory method to create a large integer from string
    [[nodiscard]] static std::optional<large_integer> from_string(const std::string& str_);

    // Constructors
    large_integer();
    large_integer(std::integral auto value_);
    large_integer(bool sign_, std::vector<underlying_type> data_);

    // Operators
    [[nodiscard]] large_integer operator-() const;
    [[nodiscard]] large_integer operator+(const large_integer& other_) const;
    [[nodiscard]] large_integer operator-(const large_integer& other_) const;
    [[nodiscard]] large_integer operator*(const large_integer& other_) const;
    [[nodiscard]] std::strong_ordering operator<=>(const large_integer& other_) const;
    [[nodiscard]] bool operator==(const large_integer& rhs_) const;

    [[nodiscard]] bool get_sign() const;
    [[nodiscard]] const collection_type& get_data() const;

private:
    // Handle the -0 case
    void fix_minus_zeros();

    // Convert an integral value to raw data
    [[nodiscard]] static collection_type to_data_collection(std::integral auto value_);

    // Helper class to avoid data copies
    class data_ref;

    // ------------------------------------------------------------------------

    bool sign{};
    collection_type data;
};

// ----------------------------------------------------------------------------

large_integer::large_integer(std::integral auto value_)
    : large_integer(value_ < 0, to_data_collection(value_)) {}

// ------------------------------------------------------------------------

[[nodiscard]] large_integer::collection_type large_integer::to_data_collection(std::integral auto value_) {
    std::vector<underlying_type> data;
    if constexpr (std::signed_integral<decltype(value_)>) {
        value_ = std::abs(value_);
    }

    if (sizeof(decltype(value_)) > sizeof(underlying_type)) {
        auto max_value = std::numeric_limits<underlying_type>::max();
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

// ----------------------------------------------------------------------------
// Free functions

[[nodiscard]] std::string to_string(const large_integer& value_);

std::istream& operator>>(std::istream& stream_, large_integer& value_);
std::ostream& operator<<(std::ostream& stream_, const large_integer& value_);

#endif // LARGE_INTEGER_HPP
#include "square_root.hpp"

[[nodiscard]] unsigned int get_next_digit_to_evaluate(unsigned int x_, bool smaller_than_current_remainder_) {
    //      5
    //    3   7
    //   1 4 6 8
    //  0 2     9
    constexpr const std::array<unsigned int, 10> next_value_if_larger{ 0, 0, 2, 1, 4, 3, 6, 6, 8, 9 };
    constexpr const std::array<unsigned int, 10> next_value_if_smaller{ 0, 2, 2, 4, 4, 7, 6, 8, 9, 9 };

    if (smaller_than_current_remainder_) {
        return next_value_if_smaller[x_];
    } else {
        return next_value_if_larger[x_];
    }
}

[[nodiscard]] std::tuple<unsigned int, large_unsigned_integer> compute_next_digit(const large_unsigned_integer& current_remainder_, const large_unsigned_integer& result_) {
    // find x * (20p + x) <= remainder*100+current
    const auto expanded_result = result_ * 20u;

    unsigned int x{ 5 };
    std::array<large_unsigned_integer, 10> sum{ 0u };
    std::array<bool, 10> smaller_than_current_remainder{ true };

    // Use dichotomic search to find the next number
    while (true) {
        // Early optimization for 0 to avoid computing the sum and the comparison as it's always 0 and smaller
        if (x == 0) {
            return { x, sum[x] };
        }

        sum[x] = (expanded_result + x) * x;
        smaller_than_current_remainder[x] = sum[x] <= current_remainder_;

        const auto next_x = get_next_digit_to_evaluate(x, smaller_than_current_remainder[x]);

        if (x == next_x) {
            if (smaller_than_current_remainder[x]) {
                return { x, sum[x] };
            } else {
                return { x - 1, sum[x - 1] };
            }
        }

        x = next_x;
    }
}

// ----------------------------------------------------------------------------

namespace details {

[[nodiscard]] unsigned int square_root_next_digit_computer::operator()(auto current_) {
    // find x * (20p + x) <= remainder*100+current
    const large_unsigned_integer current_remainder = remainder * 100u + current_;
    const auto [x, sum] = compute_next_digit(current_remainder, result);

    assert(x < 10);
    result = result * 10u + x;
    remainder = current_remainder - sum;

    return x;
}

// ----------------------------------------------------------------------------

[[nodiscard]] bool square_root_next_digit_computer::has_next_digit() const {
    return remainder != 0u;
}

// ----------------------------------------------------------------------------

generator<unsigned int> compute_fractional_part_of_square_root(square_root_next_digit_computer& computer_) {
    while (computer_.has_next_digit()) {
        constexpr const unsigned int next_value = 0;
        co_yield computer_(next_value);
    }
}

}
#include "square_root.hpp"

namespace details {

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

[[nodiscard]] std::tuple<unsigned int, large_integer> compute_next_digit(const large_integer& current_remainder_, const large_integer& result_) {
    // find x * (20p + x) <= remainder*100+current
    const auto expanded_result = result_ * 20;

    unsigned int x{ 5 };
    std::array<large_integer, 10> sum{ 0 };
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

[[nodiscard]] unsigned int square_root_digits_generator::operator()(auto current_) {
    // find x * (20p + x) <= remainder*100+current
    const large_integer current_remainder = remainder * 100 + current_;
    const auto [x, sum] = compute_next_digit(current_remainder, result);

    assert(x < 10);
    result = result * 10 + x;
    remainder = current_remainder - sum;

    return x;
}

// ----------------------------------------------------------------------------

[[nodiscard]] bool square_root_digits_generator::has_next_digit() const {
    return remainder != large_integer(0);
}

// ----------------------------------------------------------------------------

[[nodiscard]] std::string compute_fractional_part_of_square_root(unsigned int precision_, square_root_digits_generator& generator_) {
    std::string fractional_part;
    fractional_part.reserve(precision_);

    // Compute the fractional part
    while (precision_ > 0 && generator_.has_next_digit()) {
        constexpr const unsigned int next_value = 0;
        fractional_part += static_cast<std::string::value_type>(generator_(next_value)) + '0';
        --precision_;
    }

    return fractional_part;
}

// ----------------------------------------------------------------------------

[[nodiscard]] std::tuple<std::string, bool> propagate_carry(std::string&& number_, bool carry_) {
    for (auto& digit : number_ | std::views::reverse) {
        if (!carry_) {
            break;
        }

        if (digit == '9') {
            digit = '0';
        } else {
            digit += 1;
            carry_ = false;
        }
    }

    return { number_, carry_ };
}

// ----------------------------------------------------------------------------

[[nodiscard]] std::tuple<std::string, std::string> round_last_digit(std::string&& integral_part_, std::string&& fractional_part_, unsigned int rounding_digit_) {
    const double rounded_last_digit = std::round((static_cast<double>(rounding_digit_) / 10.0));

    bool carry = rounded_last_digit >= 1;
    std::tie(fractional_part_, carry) = propagate_carry(std::move(fractional_part_), carry);
    std::tie(integral_part_, carry) = propagate_carry(std::move(integral_part_), carry);

    if (carry) {
        integral_part_.insert(std::begin(integral_part_), '1');
    }

    return { integral_part_, fractional_part_ };
}

// ----------------------------------------------------------------------------

[[nodiscard]] std::string trim_lower_zeros(std::string&& fractional_part_) {
    auto last = std::ranges::find_if_not(fractional_part_ | std::views::reverse, [](auto digit) {return digit == '0';});
    fractional_part_.erase(last.base(), std::end(fractional_part_));

    return fractional_part_;
}

// ----------------------------------------------------------------------------

void compute_fractional_part_of_square_root(std::ostream& stream_, square_root_digits_generator& generator_, std::stop_token stop_) {
    while (!stop_.stop_requested() && generator_.has_next_digit()) {
        constexpr const unsigned int next_value = 0;
        stream_ << generator_(next_value) << std::flush;    // Flush stream for smoother display
    }

    return;
}

}
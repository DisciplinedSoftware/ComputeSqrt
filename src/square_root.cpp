#include "square_root.hpp"

namespace details {

[[nodiscard]] std::tuple<unsigned int, large_integer> compute_next_digit(const large_integer& current_remainder_, const large_integer& result_) {
    unsigned int x{ 0 };
    large_integer sum{ 0 };
    large_integer next_sum{ 0 };
    const auto expanded_result = result_ * 20;
    while (next_sum <= current_remainder_) {
        sum = next_sum;
        ++x;
        next_sum = (expanded_result + x) * x;
    }

    // Went one step too far
    --x;

    return { x, sum };
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
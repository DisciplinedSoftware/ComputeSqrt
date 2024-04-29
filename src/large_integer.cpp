#include "large_integer.hpp"

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

std::ostream& operator<<(std::ostream& stream_, const large_integer& value_) {
    stream_ << to_string(value_);
    return stream_;
}
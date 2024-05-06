#include <concepts>

// ----------------------------------------------------------------------------

inline auto to_char(std::integral auto value_) {
    return static_cast<char>(value_ + '0');
}

// ----------------------------------------------------------------------------

inline auto to_value(char char_) {
    return char_ - '0';
}
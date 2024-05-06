#include <concepts>
#include <string>

// ----------------------------------------------------------------------------

inline auto to_char(std::integral auto value_) {
    return static_cast<std::string::value_type>(value_ + '0');
}

// ----------------------------------------------------------------------------

inline auto to_value(std::string::value_type char_) {
    return char_ - '0';
}
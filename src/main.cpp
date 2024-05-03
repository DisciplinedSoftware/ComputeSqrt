// Output the square root of 42 in different ways

#include <charconv>
#include <chrono>
#include <format>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <ratio>
#include <string>
#include <thread>

#include <catch2/catch_session.hpp>

#include "large_integer.hpp"
#include "square_root.hpp"

// ----------------------------------------------------------------------------
// Print value to the console with the specified format using std::printf
template <typename T>
void print_using_std_printf(const char* format_, unsigned int counter_, T value_) {
    int n = std::printf(format_, counter_, value_);

    if (n < 0) {
        std::cout << "Error while printing value\n";
    }
}

// ----------------------------------------------------------------------------
// Utility
void wait_for_enter_to_be_pressed() {
    char c;
    std::cin.get(c);
}

// ----------------------------------------------------------------------------
int main(int argc, const char* argv[]) {
    const int result = Catch::Session().run(argc, argv);

    // Adjust the number of digits to the number of significant digits for a long double
    std::cout << std::setprecision(std::numeric_limits<long double>::max_digits10);

    unsigned int counter = 0;
    std::cout << ++counter << ". Using a defined constant: " << SQRT42 << '\n';

    std::cout << ++counter << ". Using cout stream operator with a constexpr constant as an integer: " << sqrt42<int> << '\n';
    std::cout << ++counter << ". Using cout stream operator with a constexpr constant as a float: " << sqrt42<float> << '\n';
    std::cout << ++counter << ". Using cout stream operator with a constexpr constant as a double: " << sqrt42<double> << '\n';
    std::cout << ++counter << ". Using cout stream operator with a constexpr constant as a long double: " << sqrt42<long double> << '\n';

    print_using_std_printf("%d. Using std::printf with fix-point notation and float: %.9f\n", ++counter, sqrt42<float>); // This is useless as f consider the argument as a double
    print_using_std_printf("%d. Using std::printf with fix-point notation and double: %.17lf\n", ++counter, sqrt42<double>);
    print_using_std_printf("%d. Using std::printf with fix-point notation and long double: %.17Lf\n", ++counter, sqrt42<long double>);

    print_using_std_printf("%d. Using std::printf with scientific notation with float: %.9e\n", ++counter, sqrt42<float>); // This is useless as f consider the argument as a double
    print_using_std_printf("%d. Using std::printf with scientific notation with double: %.17le\n", ++counter, sqrt42<double>);
    print_using_std_printf("%d. Using std::printf with scientific notation with long double: %.17Le\n", ++counter, sqrt42<long double>);

    print_using_std_printf("%d. Using std::printf with hexadecimal notation with float: %a\n", ++counter, sqrt42<float>);
    print_using_std_printf("%d. Using std::printf with hexadecimal notation with double: %la\n", ++counter, sqrt42<double>);
    print_using_std_printf("%d. Using std::printf with hexadecimal notation with long double: %La\n", ++counter, sqrt42<long double>);

    std::cout << ++counter << ". Using stream operator with a std::sqrt(float): " << std::sqrt(42.0f) << '\n';
    std::cout << ++counter << ". Using stream operator with a std::sqrt(double): " << std::sqrt(42.0) << '\n';
    std::cout << ++counter << ". Using stream operator with a std::sqrt(long double): " << std::sqrt(42.0l) << '\n';
    std::cout << ++counter << ". Using stream operator with a std::sqrtf(float): " << std::sqrt(42.0f) << '\n';
    std::cout << ++counter << ". Using stream operator with a std::sqrtl(long double): " << std::sqrt(42.0l) << '\n';

    std::cout << std::format("{:}. Using std::format with fix-point notation and float: {:.9f}\n", ++counter, sqrt42<float>);
    std::cout << std::format("{:}. Using std::format with fix-point notation and double: {:.17f}\n", ++counter, sqrt42<double>);
    std::cout << std::format("{:}. Using std::format with fix-point notation and long double: {:.17f}\n", ++counter, sqrt42<long double>);

    std::cout << std::format("{:}. Using std::format with scientific notation and float: {:.9e}\n", ++counter, sqrt42<float>);
    std::cout << std::format("{:}. Using std::format with scientific notation and double: {:.17e}\n", ++counter, sqrt42<double>);
    std::cout << std::format("{:}. Using std::format with scientific notation and long double: {:.17e}\n", ++counter, sqrt42<long double>);

    std::cout << std::format("{:}. Using std::format with hexadecimal notation and float: {:a}\n", ++counter, sqrt42<float>);
    std::cout << std::format("{:}. Using std::format with hexadecimal notation and double: {:a}\n", ++counter, sqrt42<double>);
    std::cout << std::format("{:}. Using std::format with hexadecimal notation and long double: {:a}\n", ++counter, sqrt42<long double>);

    std::cout << ++counter << ". Using std::pow: " << std::pow(42.0l, 0.5l) << '\n';

#ifdef __GNUC__
    std::cout << ++counter << ". Using assembly fsqrt: " << compute_square_root_assembly_method(42.0) << '\n';
#endif

    constexpr auto constexpr_result = compute_square_root_heron_method_constexpr(42);
    std::cout << ++counter << ". Using custom function using constexpr Heron's method: " << constexpr_result << '\n';

    std::cout << ++counter << ". Using custom function using Newton's method: " << compute_square_root_binary_search_method(42) << '\n';
    std::cout << ++counter << ". Using custom function using Heron's method: " << compute_square_root_heron_method(42) << '\n';
    std::cout << ++counter << ". Using custom function using Bakhshali's method: " << compute_square_root_bakhshali_method(42) << '\n';

    std::cout << ++counter << ". Using infinite digits (only show 1k): " << compute_square_root_digit_by_digit_method(42, 1'000) << '\n';

    const auto start = std::chrono::high_resolution_clock::now();
    const auto value = compute_square_root_digit_by_digit_method(42, 5'000);
    const auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "time taken for 5'000 digits: " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start) << '\n';

    std::cout << ++counter << ". Using infinite digits streaming:\n";

    std::cout << "Press Enter to start streaming and Enter to quit...\n";
    wait_for_enter_to_be_pressed();

    std::jthread worker([](std::stop_token stop_) { compute_square_root_digit_by_digit_method(std::cout, 42, stop_); });

    wait_for_enter_to_be_pressed();
    worker.request_stop();

    return result;
}

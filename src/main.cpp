// Output the square root of 42 in different ways

#include <iostream>
#include <thread>

#include <catch2/catch_session.hpp>

#include "square_root.hpp"

// ----------------------------------------------------------------------------
// Utility
void wait_for_enter_to_be_pressed() {
    char c;
    std::cin.get(c);
}

// ----------------------------------------------------------------------------
int main(int argc, const char* argv[]) {
    const int result = Catch::Session().run(argc, argv);

    std::cout << "Press Enter to start streaming and Enter to quit...\n";
    wait_for_enter_to_be_pressed();

    std::jthread worker([](std::stop_token stop_) { compute_square_root_digit_by_digit_method(std::cout, 42, stop_); });

    wait_for_enter_to_be_pressed();
    worker.request_stop();

    return result;
}

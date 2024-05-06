#include "../generator.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

generator<int> make_finite_generator(int begin, int end) {
    for (int i = begin; i < end; ++i) {
        co_yield i;
    }
}

generator<int> make_infinite_generator(int begin) {
    while (true) {
        co_yield begin++;
    }
}

TEST_CASE("Generator") {
    SECTION("Generate a finite stream of data") {
        auto finite_generator = make_finite_generator(0, 5);
        std::vector<int> result;
        while (finite_generator.has_value()) {
            const auto value = finite_generator.value();
            result.emplace_back(value);
        }

        CHECK(result == std::vector<int>({ 0, 1, 2, 3, 4 }));
    }

    SECTION("Generate an infinite stream of data") {
        auto infinite_generator = make_infinite_generator(0);
        std::vector<int> result;
        while (infinite_generator.has_value() && result.size() < 5) {
            const auto value = infinite_generator.value();
            result.emplace_back(value);
        }

        CHECK(result == std::vector<int>({ 0, 1, 2, 3, 4 }));
    }
}
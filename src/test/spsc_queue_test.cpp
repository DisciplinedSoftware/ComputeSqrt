#include "../spsc_queue.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Single producer single consumer queue") {
    SECTION("Empty on construction") {
        spsc_queue<int> queue;
        CHECK(queue.empty());
    }

    SECTION("Not empty after emplace") {
        spsc_queue<int> queue;
        queue.emplace(1);
        CHECK(!queue.empty());
    }

    SECTION("Emplace value is the popped one") {
        spsc_queue<int> queue;
        queue.emplace(1);
        REQUIRE(!queue.empty());

        CHECK(queue.pop() == 1);
    }

    SECTION("Queue is empty after last value is popped") {
        spsc_queue<int> queue;
        queue.emplace(1);
        REQUIRE(!queue.empty());

        std::ignore = queue.pop();
        CHECK(queue.empty());
    }

    SECTION("Emplace values are popped in the same order") {
        spsc_queue<int> queue;
        queue.emplace(1);
        queue.emplace(2);
        REQUIRE(!queue.empty());

        CHECK(queue.pop() == 1);
        CHECK(queue.pop() == 2);
    }

    SECTION("Emplace values are popped in the same order in multithreaded context") {
        constexpr size_t nb_data = 10;
        spsc_queue<int> queue;
        std::jthread producer([&]() {
            for (int i = 0; i < nb_data; ++i) {
                queue.emplace(i);
            }
        });

        while (queue.empty()) {
            std::this_thread::yield();
        }

        std::vector<int> result;
        result.reserve(nb_data);
        std::jthread consumer([&]() {
            for (int i = 0; i < nb_data; ++i) {
                result.emplace_back(queue.pop());
            }
        });

        producer.join();
        consumer.join();

        CHECK(result == std::vector<int>({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }));
    }

    SECTION("Emplace values are popped in the same order in multithreaded context with reallocation") {
        constexpr size_t nb_data = 10;
        spsc_queue<int> queue(nb_data / 2); // Force future reallocation
        std::jthread producer([&]() {
            for (int i = 0; i < nb_data; ++i) {
                queue.emplace(i);
            }
        });

        while (queue.empty()) {
            std::this_thread::yield();
        }

        std::vector<int> result;
        result.reserve(nb_data);
        std::jthread consumer([&]() {
            for (int i = 0; i < nb_data; ++i) {
                result.emplace_back(queue.pop());
            }
        });

        producer.join();
        consumer.join();

        CHECK(result == std::vector<int>({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }));
    }
}
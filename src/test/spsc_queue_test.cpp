#include <atomic>
#include <memory>
#include <thread>
#include <vector>

template< typename T >
class spsc_queue {
public:
    static constexpr size_t DefaultCapacity = 256;

    spsc_queue() : collection(std::make_shared<std::vector<T>>(DefaultCapacity)) {}

    spsc_queue(size_t capacity_) : collection(std::make_shared<std::vector<T>>(capacity_)) {}

    template< typename... Args >
    void emplace(Args&&... args) {
        size_t const current_producer_index = producer_index.load(std::memory_order_relaxed);
        size_t const current_consumer_index = consumer_index.load(std::memory_order_acquire);

        size_t next_producer_index = next_index(current_producer_index);
        if (next_producer_index == current_consumer_index) [[unlikely]] {
            // Queue is nearly full
            resize();

            // Reprocess next producer index as the collection size as changed
            next_producer_index = next_index(current_producer_index);
        }

        (*(collection.load(std::memory_order_relaxed)))[current_producer_index] = T(std::forward<Args>(args)...);
        producer_index.store(next_producer_index, std::memory_order_release);
    }

    [[nodiscard]] T pop() {
        size_t const current_consumer_index = consumer_index.load(std::memory_order_relaxed);
        size_t current_producer_index = producer_index.load(std::memory_order_acquire);

        while (empty(current_consumer_index, current_producer_index)) {
            std::this_thread::yield();
            current_producer_index = producer_index.load(std::memory_order_acquire);
        }
        T data = (*(collection.load(std::memory_order_acquire)))[current_consumer_index];
        size_t const next_consumer_index = next_index(current_consumer_index);
        consumer_index.store(next_consumer_index, std::memory_order_release);

        return data;
    }

    [[nodiscard]] bool empty() {
        size_t const current_consumer_index = consumer_index.load(std::memory_order_acquire);
        size_t const current_producer_index = producer_index.load(std::memory_order_acquire);
        return empty(current_consumer_index, current_producer_index);
    }

private:
    [[nodiscard]] inline std::size_t next_index(size_t const& index_) {
        return (index_ + 1) % collection.load()->size();
    }

    [[nodiscard]] inline bool empty(size_t current_consumer_index_, size_t current_producer_index_) {
        return current_consumer_index_ == current_producer_index_;
    }

    void resize() {
        auto old_collection = collection.load(std::memory_order_relaxed);
        auto new_collection = std::make_shared<std::vector<T>>();
        new_collection->reserve(old_collection->size() * 2);
        std::copy(std::begin(*old_collection), std::end(*old_collection), std::back_inserter(*new_collection));
        auto size_delta = new_collection->capacity() - new_collection->size();
        for (size_t i = 0; i < size_delta; ++i) {
            new_collection->emplace_back();
        }
        collection.store(new_collection, std::memory_order_relaxed);
    }

    std::atomic_size_t producer_index{ 0 };
    std::atomic_size_t consumer_index{ 0 };

    std::atomic< std::shared_ptr< std::vector< T > > > collection{};
};


//#include "../spsc_queue.hpp"

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
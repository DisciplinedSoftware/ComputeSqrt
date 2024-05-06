#include <atomic>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

template< typename T >
class spsc_queue {
public:
    static constexpr size_t DefaultCapacity = 2048;

    spsc_queue()
        : collection(std::make_shared< std::vector< T > >(DefaultCapacity)) {}

    spsc_queue(size_t capacity_)
        : collection(std::make_shared< std::vector< T > >(capacity_)) {}

    template< typename ... Args >
    void emplace(Args&&... args) {
        size_t       current_producer_index = producer_index.load(std::memory_order_relaxed);
        size_t const current_consumer_index = consumer_index.load(std::memory_order_acquire);

        size_t next_producer_index = next_index(current_producer_index);
        if (next_producer_index == current_consumer_index) [[unlikely]] {
            // Queue is nearly full
            std::tie(current_producer_index, next_producer_index) = resize(current_consumer_index);
        }

        (*(collection.load(std::memory_order_relaxed)))[current_producer_index] = T(std::forward< Args >(args)...);
        producer_index.store(next_producer_index, std::memory_order_release);
    }

    [[nodiscard]] T pop() {
        size_t const current_consumer_index = consumer_index.load(std::memory_order_relaxed);
        size_t       current_producer_index = producer_index.load(std::memory_order_acquire);

        while (empty(current_consumer_index, current_producer_index)) {
            std::this_thread::yield();
            current_producer_index = producer_index.load(std::memory_order_acquire);
        }

        T data = (*(collection.load(std::memory_order_acquire)))[current_consumer_index];
        size_t const next_consumer_index = next_index(current_consumer_index);
        consumer_index.store(next_consumer_index, std::memory_order_release);

        return data;
    }

    // Will pop data even if stop is requested to allow emptying the queue
    [[nodiscard]] std::optional< T > pop(std::stop_token stop_) {
        size_t const current_consumer_index = consumer_index.load(std::memory_order_relaxed);
        size_t       current_producer_index = producer_index.load(std::memory_order_acquire);

        while (empty(current_consumer_index, current_producer_index)) {
            if (stop_.stop_requested()) [[unlikely]] {
                return {};
            }

            std::this_thread::yield();
            current_producer_index = producer_index.load(std::memory_order_acquire);
        }

        T data = (*(collection.load(std::memory_order_acquire)))[current_consumer_index];
        size_t const next_consumer_index = next_index(current_consumer_index);
        consumer_index.store(next_consumer_index, std::memory_order_release);

        return std::make_optional< T >(data);
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

    // Return the new producer index and the next producer index
    [[nodiscard]] std::tuple< size_t, size_t > resize(size_t current_consumer_index_) {
        auto old_collection = collection.load(std::memory_order_acquire);
        auto new_collection = std::make_shared< std::vector< T > >(old_collection->size() + DefaultCapacity); // Increment size slightly

        std::copy(std::begin(*old_collection) + current_consumer_index_, std::end(*old_collection), std::begin(*new_collection) + current_consumer_index_);
        std::copy(std::begin(*old_collection), std::begin(*old_collection) + current_consumer_index_, std::begin(*new_collection) + old_collection->size());

        collection.store(new_collection, std::memory_order_release);

        auto new_current_producer_index = old_collection->size() + current_consumer_index_ - 1;
        return { new_current_producer_index, new_current_producer_index + 1 };
    }

    std::atomic_size_t producer_index{ 0 };
    std::atomic_size_t consumer_index{ 0 };

    std::atomic< std::shared_ptr< std::vector< T > > > collection{};
};
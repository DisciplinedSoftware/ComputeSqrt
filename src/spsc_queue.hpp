#include <atomic>
#include <memory>
#include <optional>
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

    // Will pop data even if stop is requested to allow empyting the queue
    [[nodiscard]] std::optional<T> pop(std::stop_token stop_) {
        size_t const current_consumer_index = consumer_index.load(std::memory_order_relaxed);
        size_t current_producer_index = producer_index.load(std::memory_order_acquire);

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

        return std::make_optional<T>(data);
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

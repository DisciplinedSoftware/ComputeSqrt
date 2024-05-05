#include <coroutine>
#include <exception>

template<typename T>
class generator {
public:
    class promise_type;
    using handle_type = std::coroutineutine_handle<promise_type>;

    generator(handle_type handle_)
        : coroutine(handle_) {}

    handle_type coroutine;

    ~generator() {
        if (coroutine) {
            coroutine.destroy();
        }
    }

    generator(const generator&) = delete;
    generator& operator=(const generator&) = delete;

    generator(generator&& other_) noexcept
        : coroutine(std::exchange(other_.coroutine, nullptr)) {}

    generator& operator=(generator&& other_) noexcept {
        coroutine = std::exchange(other_.coroutine, nullptr);
        return *this;
    }

    [[nodiscard]] T get_value() {
        return coroutine.promise().current_value;
    }

    [[nodiscard]] bool has_next() {
        coroutine.resume();
        return !coroutine.done();
    }

    class promise_type {
    public:
        promise_type() = default;

        auto initial_suspend() { return std::suspend_always{}; }
        auto final_suspend() noexcept { return std::suspend_always{}; }
        auto get_return_object() {
            return generator{ handle_type::from_promise(*this) };
        }

        auto yield_value(int value) {
            current_value = value;
            return std::suspend_always{};
        }

        void return_void() {}
        void unhandled_exception() {
            std::rethrow_exception(std::current_exception());
        }

        T current_value{};
    };
};


//#include "../generator.hpp"

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
        while (finite_generator.has_next()) {
            const auto value = finite_generator.get_value();
            result.emplace_back(value);
        }

        CHECK(result == std::vector<int>({ 0, 1, 2, 3, 4 }));
    }

    SECTION("Generate an infinite stream of data") {
        auto infinite_generator = make_infinite_generator(0);
        std::vector<int> result;
        while (infinite_generator.has_next() && result.size() < 5) {
            const auto value = infinite_generator.get_value();
            result.emplace_back(value);
        }

        CHECK(result == std::vector<int>({ 0, 1, 2, 3, 4 }));
    }
}
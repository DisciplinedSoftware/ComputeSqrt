#include <coroutine>
#include <exception>
#include <utility>

template<typename T>
class generator {
public:
    class promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

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

    [[nodiscard]] T value() {
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
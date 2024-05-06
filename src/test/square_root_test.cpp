#include "../square_root.hpp"

#include <thread>
#include <sstream>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Square root") {
    SECTION("compute_square_root_digit_by_digit_method with stream") {
        using namespace std::string_literals;
        std::ostringstream stream;
        std::stop_token stop;

        compute_square_root_digit_by_digit_method(stream, -1, stop);
        CHECK(stream.str() == "nan"s);
        stream = std::ostringstream();
        compute_square_root_digit_by_digit_method(stream, 0, stop);
        CHECK(stream.str() == "0"s);
        stream = std::ostringstream();
        compute_square_root_digit_by_digit_method(stream, -0, stop);
        CHECK(stream.str() == "0"s);
        stream = std::ostringstream();
        compute_square_root_digit_by_digit_method(stream, 1, stop);
        CHECK(stream.str() == "1"s);
        stream = std::ostringstream();
        compute_square_root_digit_by_digit_method(stream, 4, stop);
        CHECK(stream.str() == "2"s);
        stream = std::ostringstream();

        std::jthread thread([&stream](std::stop_token stop_) {compute_square_root_digit_by_digit_method(stream, 42, stop_);});
        while (stream.str().length() < 102) { std::this_thread::yield(); }
        thread.request_stop();
        thread.join();
        auto result = stream.str().substr(0, 102);
        CHECK(result == "6.4807406984078602309659674360879966577052043070583465497113543978096173778440443714003609066056102356"s);
    }
}
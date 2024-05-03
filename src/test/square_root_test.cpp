#include "../square_root.hpp"

#include <thread>
#include <sstream>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Square root") {
    SECTION("compute_square_root_binary_search_method") {
        CHECK(compute_square_root_binary_search_method(0) == std::sqrt(0));
        CHECK(compute_square_root_binary_search_method(-0) == std::sqrt(-0));
        CHECK(compute_square_root_binary_search_method(1) == std::sqrt(1));
        CHECK(compute_square_root_binary_search_method(4) == std::sqrt(4));
        CHECK(compute_square_root_binary_search_method(2) == Catch::Approx(std::numbers::sqrt2).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_binary_search_method(780.14) == Catch::Approx(std::sqrt(780.14)).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_binary_search_method(0.5) == Catch::Approx(std::sqrt(0.5)).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_binary_search_method(42) == Catch::Approx(SQRT42 /*std::sqrt(42)*/).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_binary_search_method(1e-15) == Catch::Approx(std::sqrt(1e-15)).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_binary_search_method(1e-300) == Catch::Approx(std::sqrt(1e-300)).epsilon(std::numeric_limits<double>::epsilon()));
    }

    SECTION("compute_square_root_heron_method") {
        CHECK(compute_square_root_heron_method(0) == std::sqrt(0));
        CHECK(compute_square_root_heron_method(-0) == std::sqrt(-0));
        CHECK(compute_square_root_heron_method(1) == std::sqrt(1));
        CHECK(compute_square_root_heron_method(4) == std::sqrt(4));
        CHECK(compute_square_root_heron_method(2) == Catch::Approx(std::numbers::sqrt2).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_heron_method(780.14) == Catch::Approx(std::sqrt(780.14)).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_heron_method(0.5) == Catch::Approx(std::sqrt(0.5)).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_heron_method(42) == Catch::Approx(SQRT42 /*std::sqrt(42)*/).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_heron_method(1e-15) == Catch::Approx(std::sqrt(1e-15)).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_heron_method(2.2e-300) == Catch::Approx(std::sqrt(2.2e-300)).epsilon(std::numeric_limits<double>::epsilon()));
    }

    SECTION("compute_square_root_bakhshali_method") {
        CHECK(compute_square_root_bakhshali_method(0) == std::sqrt(0));
        CHECK(compute_square_root_bakhshali_method(-0) == std::sqrt(-0));
        CHECK(compute_square_root_bakhshali_method(1) == std::sqrt(1));
        CHECK(compute_square_root_bakhshali_method(4) == std::sqrt(4));
        CHECK(compute_square_root_bakhshali_method(2) == Catch::Approx(std::numbers::sqrt2).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_bakhshali_method(780.14) == Catch::Approx(std::sqrt(780.14)).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_bakhshali_method(0.5) == Catch::Approx(std::sqrt(0.5)).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_bakhshali_method(42) == Catch::Approx(SQRT42 /*std::sqrt(42)*/).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_bakhshali_method(1e-15) == Catch::Approx(std::sqrt(1e-15)).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_bakhshali_method(1e-300) == Catch::Approx(std::sqrt(1e-300)).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_bakhshali_method(2.2e-300) == Catch::Approx(std::sqrt(2.2e-300)).epsilon(std::numeric_limits<double>::epsilon()));
    }

    SECTION("compute_square_root_digit_by_digit_method") {
        using namespace std::string_literals;
        CHECK(compute_square_root_digit_by_digit_method(-1, 2) == "nan"s);
        CHECK(compute_square_root_digit_by_digit_method(0, 0) == "0"s);
        CHECK(compute_square_root_digit_by_digit_method(-0, 0) == "0"s);
        CHECK(compute_square_root_digit_by_digit_method(1, 0) == "1"s);
        CHECK(compute_square_root_digit_by_digit_method(4, 0) == "2"s);
        CHECK(compute_square_root_digit_by_digit_method(2, 31) == "1.4142135623730950488016887242097"s);
        CHECK(compute_square_root_digit_by_digit_method(42, 30) == "6.480740698407860230965967436088"s);
        CHECK(compute_square_root_digit_by_digit_method(42, 100) == "6.4807406984078602309659674360879966577052043070583465497113543978096173778440443714003609066056102357"s);

        CHECK(compute_square_root_digit_by_digit_method(99, 0) == "10"s);
        CHECK(compute_square_root_digit_by_digit_method(99, 1) == "9.9"s);
        CHECK(compute_square_root_digit_by_digit_method(9999, 0) == "100"s);
        CHECK(compute_square_root_digit_by_digit_method(999999, 1) == "1000"s);
    }

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

#ifdef __GNUC__
    SECTION("compute_square_root_assembly_method") {
        CHECK(compute_square_root_assembly_method(0) == std::sqrt(0));
        CHECK(compute_square_root_assembly_method(-0) == std::sqrt(-0));
        CHECK(compute_square_root_assembly_method(1) == std::sqrt(1));
        CHECK(compute_square_root_assembly_method(4) == std::sqrt(4));
        CHECK(compute_square_root_assembly_method(2) == Catch::Approx(std::numbers::sqrt2).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_assembly_method(780.14) == Catch::Approx(std::sqrt(780.14)).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_assembly_method(0.5) == Catch::Approx(std::sqrt(0.5)).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_assembly_method(42) == Catch::Approx(SQRT42 /*std::sqrt(42)*/).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_assembly_method(1e-15) == Catch::Approx(std::sqrt(1e-15)).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_assembly_method(1e-300) == Catch::Approx(std::sqrt(1e-300)).epsilon(std::numeric_limits<double>::epsilon()));
        CHECK(compute_square_root_assembly_method(2.2e-300) == Catch::Approx(std::sqrt(2.2e-300)).epsilon(std::numeric_limits<double>::epsilon()));
    }
#endif // __GNUC__
}
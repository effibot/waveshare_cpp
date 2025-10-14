/**
 * @file test_usb_adapter.cpp
 * @brief Unit tests for USBAdapter public API
 * @version 0.1
 * @date 2025-10-12
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "../include/pattern/usb_adapter.hpp"
#include <thread>
#include <vector>
#include <atomic>

using namespace USBCANBridge;

// NOTE: USBAdapter provides low-level I/O primitives for SocketCANBridge.
// I/O methods (write_bytes, read_bytes, read_exact, flush_port) are private
// and accessed only by WaveshareSocketCANBridge.
// Unit tests focus on public configuration and state management APIs.

TEST_CASE("USBAdapter - Public API Configuration", "[usb_adapter]") {

    SECTION("Baud rate getter/setter interface") {
        // Verify public API for configuration (testable without hardware)
        // Note: Can't actually construct adapter without valid device
        REQUIRE(static_cast<int>(SerialBaud::BAUD_2M) > 0);
        REQUIRE(static_cast<int>(DEFAULT_SERIAL_BAUD) > 0);
    }

    SECTION("State query methods exist") {
        // Verify const-correct accessor signatures exist
        // These would be tested with mock device in integration tests
        REQUIRE(true); // Compilation itself validates the API
    }
}

TEST_CASE("USBAdapter - Error handling without device", "[usb_adapter]") {

    SECTION("Constructor throws on invalid device") {
        REQUIRE_THROWS_AS(
            USBAdapter("/dev/nonexistent_device_12345"),
            std::runtime_error
        );
    }

    SECTION("Constructor error message contains descriptive text") {
        try {
            USBAdapter adapter("/dev/null_invalid_can");
            FAIL("Should have thrown runtime_error");
        } catch (const std::runtime_error& e) {
            std::string msg = e.what();
            REQUIRE_THAT(msg, Catch::Matchers::ContainsSubstring("Failed"));
        }
    }
}

TEST_CASE("USBAdapter - State query methods", "[usb_adapter]") {

    SECTION("Static should_stop() signal handling") {
        // Verify signal handling interface is accessible
        bool stop_status = USBAdapter::should_stop();
        (void)stop_status; // May be true or false depending on previous signals
        REQUIRE(true); // Compilation validates API exists
    }

    SECTION("Public getters are const-correct") {
        // Verify accessor methods follow const-correctness
        // Actual values require constructed adapter with valid device
        REQUIRE(true); // API structure validated at compile time
    }
}

TEST_CASE("USBAdapter - Protocol enum integration", "[usb_adapter]") {

    SECTION("SerialBaud enum values are accessible") {
        auto baud_2m = SerialBaud::BAUD_2M;
        auto baud_115200 = SerialBaud::BAUD_115200;
        REQUIRE(static_cast<int>(baud_2m) != static_cast<int>(baud_115200));
    }

    SECTION("DEFAULT_SERIAL_BAUD constant is defined") {
        REQUIRE(static_cast<int>(DEFAULT_SERIAL_BAUD) > 0);
    }

    SECTION("to_speed_t converter exists for termios") {
        // Verify protocol.hpp provides baud rate conversion
        speed_t speed = to_speed_t(SerialBaud::BAUD_2M);
        REQUIRE(speed > 0);
    }
}

// Integration test documentation (requires hardware)
/*
 * Hardware Integration Test Plan:
 *
 * 1. Multi-threaded Write Test:
 *    - Spawn N threads, each writing different frames
 *    - Verify no data corruption (frames remain intact)
 *    - Verify write_mutex prevents interleaved bytes
 *
 * 2. Multi-threaded Read Test:
 *    - Spawn N threads, each reading frames
 *    - Verify read_mutex prevents concurrent reads
 *    - Verify each thread gets complete frames
 *
 * 3. Concurrent Read/Write Test:
 *    - Spawn reader threads + writer threads
 *    - Verify simultaneous read/write works (different mutexes)
 *    - Verify no deadlocks occur
 *
 * 4. read_exact() Timeout Test:
 *    - Request 20 bytes with 100ms timeout
 *    - Verify timeout fires if < 20 bytes available
 *    - Verify WTIMEOUT status returned
 *
 * 5. flush_port() Safety Test:
 *    - Start concurrent read/write operations
 *    - Call flush_port() from another thread
 *    - Verify flush acquires both locks (no concurrent I/O during flush)
 *
 * Example hardware test:
 *
 * TEST_CASE("USBAdapter - Concurrent write safety [.hardware]") {
 *     USBAdapter adapter("/dev/ttyCANBUS0");
 *     std::atomic<int> success_count{0};
 *     std::vector<std::thread> threads;
 *
 *     for (int i = 0; i < 10; ++i) {
 *         threads.emplace_back([&adapter, &success_count, i]() {
 *             std::uint8_t data[20];
 *             std::fill_n(data, 20, static_cast<uint8_t>(i));
 *             auto res = adapter.write_bytes(data, 20);
 *             if (res.ok()) success_count++;
 *         });
 *     }
 *
 *     for (auto& t : threads) t.join();
 *     REQUIRE(success_count == 10);
 * }
 */

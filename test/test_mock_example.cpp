/**
 * @file test_mock_example.cpp
 * @brief Example demonstrating mock usage for SocketCANBridge testing
 * @version 1.0
 * @date 2025-10-14
 */

#include <catch2/catch_test_macros.hpp>
#include "mocks/mock_serial_port.hpp"
#include "mocks/mock_can_socket.hpp"
#include "pattern/socketcan_bridge.hpp"
#include "pattern/usb_adapter.hpp"
#include <thread>
#include <chrono>

using namespace waveshare;
using namespace waveshare::test;

/**
 * @brief Example test showing how to use mocks with SocketCANBridge
 *
 * This demonstrates the pattern for testing bridge without hardware:
 * 1. Create mock CAN socket and serial port
 * 2. Create USBAdapter with mock serial port (or use mock USBAdapter)
 * 3. Create SocketCANBridge with DI constructor (inject mocks)
 * 4. Inject test data via mocks
 * 5. Verify forwarding via mock history
 */
TEST_CASE("Mock Example: Bridge forwards USB→CAN", "[mock][example]") {
    // === Step 1: Create Mocks ===
    auto mock_can = std::make_unique<MockCANSocket>("vcan0", 100);
    auto mock_serial = std::make_unique<MockSerialPort>("/dev/mock");

    // === Step 2: Create USBAdapter with Mock Serial Port ===
    // Note: USBAdapter uses ISerialPort via DI, but doesn't have DI constructor yet
    // For now, we'll use the factory method and note this needs updating

    // TODO: Update USBAdapter to accept ISerialPort* in constructor for DI
    // auto usb_adapter = std::make_unique<USBAdapter>(
    //     std::move(mock_serial),
    //     "/dev/mock",
    //     SerialBaud::BAUD_2M
    // );

    // Workaround: Use real USBAdapter::create() for now
    // (This test will be fully mock-based once USBAdapter DI is complete)

    SECTION("Demonstrates mock pattern - implementation pending USBAdapter DI") {
        // This example shows the intended pattern
        // Actual implementation requires USBAdapter to accept ISerialPort* in constructor

        REQUIRE(true);  // Placeholder
    }
}

/**
 * @brief Example: Mock CAN Socket basic operations
 */
TEST_CASE("Mock CAN Socket - Basic Operations", "[mock][can_socket]") {
    MockCANSocket mock("vcan0", 100);

    SECTION("Initial state") {
        REQUIRE(mock.is_open());
        REQUIRE(mock.get_interface_name() == "vcan0");
        REQUIRE(mock.get_fd() > 0);
        REQUIRE(mock.get_rx_queue_size() == 0);
        REQUIRE(mock.get_tx_history().empty());
    }

    SECTION("Send frame") {
        can_frame frame = MockCANSocket::make_frame(0x123, {0x11, 0x22, 0x33});

        ssize_t result = mock.send(frame);

        REQUIRE(result == sizeof(can_frame));
        REQUIRE(mock.get_tx_history().size() == 1);
        REQUIRE(mock.get_tx_history()[0].can_id == 0x123);
        REQUIRE(mock.get_tx_history()[0].can_dlc == 3);
    }

    SECTION("Receive frame") {
        can_frame injected = MockCANSocket::make_frame(0x456, {0xAA, 0xBB});
        mock.inject_rx_frame(injected);

        can_frame received{};
        ssize_t result = mock.receive(received);

        REQUIRE(result == sizeof(can_frame));
        REQUIRE(received.can_id == 0x456);
        REQUIRE(received.can_dlc == 2);
        REQUIRE(received.data[0] == 0xAA);
        REQUIRE(received.data[1] == 0xBB);
    }

    SECTION("Timeout simulation") {
        mock.set_simulate_timeout(true);

        can_frame frame{};
        ssize_t result = mock.receive(frame);

        REQUIRE(result == -1);
        REQUIRE(errno == EAGAIN);
    }

    SECTION("Error simulation") {
        mock.set_simulate_send_error(true);

        can_frame frame{};
        ssize_t result = mock.send(frame);

        REQUIRE(result == -1);
        REQUIRE(errno == EIO);
    }
}

/**
 * @brief Example: Mock Serial Port basic operations
 */
TEST_CASE("Mock Serial Port - Basic Operations", "[mock][serial_port]") {
    MockSerialPort mock("/dev/mock");

    SECTION("Initial state") {
        REQUIRE(mock.is_open());
        REQUIRE(mock.get_device_path() == "/dev/mock");
        REQUIRE(mock.get_fd() > 0);
        REQUIRE(mock.get_rx_queue_size() == 0);
        REQUIRE(mock.get_tx_history().empty());
    }

    SECTION("Write data") {
        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};

        ssize_t result = mock.write(data.data(), data.size());

        REQUIRE(result == 4);
        REQUIRE(mock.get_tx_history().size() == 1);
        REQUIRE(mock.get_tx_history()[0] == data);
    }

    SECTION("Read data") {
        std::vector<uint8_t> injected = {0xAA, 0xBB, 0xCC};
        mock.inject_rx_data(injected);

        uint8_t buffer[10];
        ssize_t result = mock.read(buffer, sizeof(buffer), 100);

        REQUIRE(result == 3);
        REQUIRE(buffer[0] == 0xAA);
        REQUIRE(buffer[1] == 0xBB);
        REQUIRE(buffer[2] == 0xCC);
    }

    SECTION("Timeout simulation") {
        mock.set_simulate_timeout(true);

        uint8_t buffer[10];
        ssize_t result = mock.read(buffer, sizeof(buffer), 100);

        REQUIRE(result == -1);
        REQUIRE(errno == EAGAIN);
    }
}

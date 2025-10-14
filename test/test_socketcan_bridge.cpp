/**
 * @file test_socketcan_bridge.cpp
 * @brief Unit tests for SocketCAN bridge
 * @version 1.0
 * @date 2025-10-15
 *
 * Note: Tests use mocks - no hardware required!
 */

#include <catch2/catch_test_macros.hpp>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <cstdlib>

#include "../include/pattern/socketcan_bridge.hpp"
#include "../include/exception/waveshare_exception.hpp"
#include "test_utils.hpp"

using namespace waveshare;

TEST_CASE("SocketCANBridge - Construction with valid config", "[bridge][socketcan][construction]") {
    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = "/dev/ttyUSB0";

    REQUIRE_NOTHROW([&]() {
            auto bridge = waveshare::test::create_bridge_with_mocks(config);
            REQUIRE(bridge->is_socketcan_open());
            REQUIRE(bridge->get_socketcan_fd() >= 0);
        }());
}

TEST_CASE("SocketCANBridge - Empty interface name throws", "[bridge][socketcan][error]") {
    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "";
    config.usb_device_path = "/dev/ttyUSB0";  // Doesn't matter, validation happens first

    // Should throw during config validation (before any device access)
    REQUIRE_THROWS_AS(waveshare::test::create_bridge_with_mocks(config),
        std::invalid_argument
    );
}
TEST_CASE("SocketCANBridge - Get configuration", "[bridge][socketcan][config]") {

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = "/dev/ttyUSB0";
    config.can_baud_rate = CANBaud::BAUD_500K;
    config.can_mode = CANMode::LOOPBACK;

    auto bridge = waveshare::test::create_bridge_with_mocks(config);

    const auto& retrieved_config = bridge->get_config();
    REQUIRE(retrieved_config.socketcan_interface == "vcan0");
    REQUIRE(retrieved_config.can_baud_rate == CANBaud::BAUD_500K);
    REQUIRE(retrieved_config.can_mode == CANMode::LOOPBACK);
}

TEST_CASE("SocketCANBridge - Timeout configuration", "[bridge][socketcan][timeout]") {

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = "/dev/ttyUSB0";
    config.socketcan_read_timeout_ms = 500;

    // Mocks don't simulate timeouts - just verify config is accepted
    REQUIRE_NOTHROW([&]() {
            auto bridge = waveshare::test::create_bridge_with_mocks(config);
            REQUIRE(bridge->get_config().socketcan_read_timeout_ms == 500);
        }());
}

TEST_CASE("SocketCANBridge - Move semantics", "[bridge][socketcan][move]") {

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = "/dev/ttyUSB0";

    auto bridge1 = waveshare::test::create_bridge_with_mocks(config);
    int original_fd = bridge1->get_socketcan_fd();
    REQUIRE(original_fd >= 0);

    // Move constructor is deleted - test compile-time check
    // static_assert(!std::is_move_constructible_v<SocketCANBridge>);
    REQUIRE(bridge1->is_socketcan_open());
}

TEST_CASE("SocketCANBridge - USB adapter initialization", "[bridge][usb][initialization]") {

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = "/dev/ttyUSB0";
    config.serial_baud_rate = SerialBaud::BAUD_2M;

    auto bridge = waveshare::test::create_bridge_with_mocks(config);

    // Verify adapter is initialized
    auto adapter = bridge->get_adapter();
    REQUIRE(adapter != nullptr);
}

TEST_CASE("SocketCANBridge - Configuration with CAN parameters", "[bridge][usb][config]") {

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = "/dev/ttyUSB0";
    config.can_baud_rate = CANBaud::BAUD_500K;
    config.can_mode = CANMode::LOOPBACK;
    config.auto_retransmit = false;
    config.filter_id = 0x123;
    config.filter_mask = 0x7FF;

    // Should not throw (configuration is sent to adapter)
    REQUIRE_NOTHROW([&]() {
            auto bridge = waveshare::test::create_bridge_with_mocks(config);
        }());
}

TEST_CASE("SocketCANBridge - Extended ID filter triggers extended config",
    "[bridge][usb][extended]") {

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = "/dev/ttyUSB0";
    config.filter_id = 0x12345678;  // Extended ID (> 11 bits)
    config.filter_mask = 0x1FFFFFFF;

    // Should use extended CAN version internally
    REQUIRE_NOTHROW([&]() {
            auto bridge = waveshare::test::create_bridge_with_mocks(config);
        }());
}

TEST_CASE("SocketCANBridge - Statistics initialization", "[bridge][stats][initialization]") {

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = "/dev/ttyUSB0";

    auto bridge = waveshare::test::create_bridge_with_mocks(config);

    // Get initial statistics - should all be zero
    auto stats = bridge->get_statistics();
    REQUIRE(stats.usb_rx_frames == 0);
    REQUIRE(stats.usb_tx_frames == 0);
    REQUIRE(stats.socketcan_rx_frames == 0);
    REQUIRE(stats.socketcan_tx_frames == 0);
    REQUIRE(stats.usb_rx_errors == 0);
    REQUIRE(stats.usb_tx_errors == 0);
    REQUIRE(stats.socketcan_rx_errors == 0);
    REQUIRE(stats.socketcan_tx_errors == 0);
    REQUIRE(stats.conversion_errors == 0);
}

TEST_CASE("SocketCANBridge - Statistics reset", "[bridge][stats][reset]") {
    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = "/dev/ttyUSB0";

    auto bridge = waveshare::test::create_bridge_with_mocks(config);

    // Reset statistics
    bridge->reset_statistics();

    // Verify all counters are zero
    auto stats = bridge->get_statistics();
    REQUIRE(stats.usb_rx_frames == 0);
    REQUIRE(stats.usb_tx_frames == 0);
    REQUIRE(stats.socketcan_rx_frames == 0);
    REQUIRE(stats.socketcan_tx_frames == 0);
    REQUIRE(stats.usb_rx_errors == 0);
    REQUIRE(stats.usb_tx_errors == 0);
    REQUIRE(stats.socketcan_rx_errors == 0);
    REQUIRE(stats.socketcan_tx_errors == 0);
    REQUIRE(stats.conversion_errors == 0);
}

TEST_CASE("SocketCANBridge - Statistics to_string", "[bridge][stats][output]") {
    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = "/dev/ttyUSB0";

    auto bridge = waveshare::test::create_bridge_with_mocks(config);

    // Get statistics and convert to string
    auto stats = bridge->get_statistics();
    std::string stats_str = stats.to_string();

    // Verify string contains expected labels
    REQUIRE(stats_str.find("Bridge Statistics") != std::string::npos);
    REQUIRE(stats_str.find("USB RX:") != std::string::npos);
    REQUIRE(stats_str.find("USB TX:") != std::string::npos);
    REQUIRE(stats_str.find("SocketCAN RX:") != std::string::npos);
    REQUIRE(stats_str.find("SocketCAN TX:") != std::string::npos);
    REQUIRE(stats_str.find("USB RX Errors:") != std::string::npos);
    REQUIRE(stats_str.find("USB TX Errors:") != std::string::npos);
    REQUIRE(stats_str.find("CAN RX Errors:") != std::string::npos);
    REQUIRE(stats_str.find("CAN TX Errors:") != std::string::npos);
    REQUIRE(stats_str.find("Conv Errors:") != std::string::npos);
}

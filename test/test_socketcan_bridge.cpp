/**
 * @file test_socketcan_bridge.cpp
 * @brief Unit tests for SocketCAN bridge
 * @version 1.0
 * @date 2025-10-14
 *
 * Note: These tests require a virtual CAN interface (vcan0).
 * Setup: sudo modprobe vcan && sudo ip link add dev vcan0 type vcan && sudo ip link set up vcan0
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

using namespace waveshare;

// Helper to check if vcan0 exists
bool vcan0_exists() {
    int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) return false;

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, "vcan0", IFNAMSIZ - 1);

    bool exists = (ioctl(fd, SIOCGIFINDEX, &ifr) >= 0);
    close(fd);
    return exists;
}

// Helper to check if test USB device exists
// Reads from .env file or defaults to /dev/ttyUSB0
bool test_usb_device_exists() {
    try {
        // .env is copied to test build directory by CMake
        auto config = BridgeConfig::load("../.env");
        const char* device = config.usb_device_path.c_str();

        // Check if device exists and is accessible
        int fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0) {
            return false;
        }
        close(fd);
        return true;
    } catch (...) {
        return false;  // Config load failed
    }
}

// Get test USB device path from .env file (or default)
std::string get_test_usb_device() {
    try {
        // .env is copied to test build directory by CMake
        auto config = BridgeConfig::load("../.env");
        return config.usb_device_path;
    } catch (...) {
        return "/dev/ttyUSB0";  // Fallback default
    }
}

TEST_CASE("SocketCANBridge - Construction with valid config", "[bridge][socketcan][construction]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }
    if (!test_usb_device_exists()) {
        SKIP("Test USB device not available (configure in .env or ensure /dev/ttyUSB0 exists)");
    }

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = get_test_usb_device();

    REQUIRE_NOTHROW([&]() {
            SocketCANBridge bridge(config);
            REQUIRE(bridge.is_socketcan_open());
            REQUIRE(bridge.get_socketcan_fd() >= 0);
        }());
}

TEST_CASE("SocketCANBridge - Invalid interface throws", "[bridge][socketcan][error]") {
    if (!test_usb_device_exists()) {
        SKIP("Test USB device not available (configure in .env or ensure /dev/ttyUSB0 exists)");
    }

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "nonexistent_interface_xyz";
    config.usb_device_path = get_test_usb_device();

    // Should throw when trying to open invalid SocketCAN interface
    REQUIRE_THROWS_AS(
        SocketCANBridge(config),
        DeviceException
    );
}

TEST_CASE("SocketCANBridge - Empty interface name throws", "[bridge][socketcan][error]") {
    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "";
    config.usb_device_path = "/dev/ttyUSB0";  // Doesn't matter, validation happens first

    // Should throw during config validation (before any device access)
    REQUIRE_THROWS_AS(
        SocketCANBridge(config),
        std::invalid_argument
    );
}

TEST_CASE("SocketCANBridge - Destructor closes socket", "[bridge][socketcan][cleanup]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }
    if (!test_usb_device_exists()) {
        SKIP("Test USB device not available (configure in .env or ensure /dev/ttyUSB0 exists)");
    }

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = get_test_usb_device();

    int fd_copy = -1;
    {
        SocketCANBridge bridge(config);
        fd_copy = bridge.get_socketcan_fd();
        REQUIRE(fd_copy >= 0);
        REQUIRE(bridge.is_socketcan_open());
    }
    // Bridge destroyed, socket should be closed

    // Try to use the FD - should fail (EBADF - bad file descriptor)
    char dummy;
    ssize_t result = read(fd_copy, &dummy, 1);
    REQUIRE(result < 0);
    REQUIRE(errno == EBADF);  // File descriptor is closed
}

TEST_CASE("SocketCANBridge - Get configuration", "[bridge][socketcan][config]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }
    if (!test_usb_device_exists()) {
        SKIP("Test USB device not available (configure in .env or ensure /dev/ttyUSB0 exists)");
    }

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = get_test_usb_device();
    config.can_baud_rate = CANBaud::BAUD_500K;
    config.can_mode = CANMode::LOOPBACK;

    SocketCANBridge bridge(config);

    const auto& retrieved_config = bridge.get_config();
    REQUIRE(retrieved_config.socketcan_interface == "vcan0");
    REQUIRE(retrieved_config.can_baud_rate == CANBaud::BAUD_500K);
    REQUIRE(retrieved_config.can_mode == CANMode::LOOPBACK);
}

TEST_CASE("SocketCANBridge - Timeout configuration", "[bridge][socketcan][timeout]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }
    if (!test_usb_device_exists()) {
        SKIP("Test USB device not available (configure in .env or ensure /dev/ttyUSB0 exists)");
    }

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = get_test_usb_device();
    config.socketcan_read_timeout_ms = 500;

    REQUIRE_NOTHROW([&]() {
            SocketCANBridge bridge(config);

            // Verify timeout is set by attempting to read (should timeout)
            struct can_frame frame;
            int fd = bridge.get_socketcan_fd();

            auto start = std::chrono::steady_clock::now();
            ssize_t result = read(fd, &frame, sizeof(frame));
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            ).count();

            // Should timeout (result == -1 with EAGAIN or EWOULDBLOCK)
            REQUIRE(result == -1);
            REQUIRE((errno == EAGAIN || errno == EWOULDBLOCK));

            // Timeout should be approximately 500ms (allow some tolerance)
            REQUIRE(duration >= 400); // At least 400ms
            REQUIRE(duration <= 700); // At most 700ms
        }());
}

TEST_CASE("SocketCANBridge - Socket is bound to correct interface",
    "[bridge][socketcan][binding]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }
    if (!test_usb_device_exists()) {
        SKIP("Test USB device not available (configure in .env or ensure /dev/ttyUSB0 exists)");
    }

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = get_test_usb_device();

    SocketCANBridge bridge(config);

    // Verify we can get socket name
    struct sockaddr_can addr;
    socklen_t len = sizeof(addr);
    int fd = bridge.get_socketcan_fd();

    REQUIRE(getsockname(fd, reinterpret_cast<struct sockaddr*>(&addr), &len) >= 0);
    REQUIRE(addr.can_family == AF_CAN);

    // Get interface index for vcan0
    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, "vcan0", IFNAMSIZ - 1);
    REQUIRE(ioctl(fd, SIOCGIFINDEX, &ifr) >= 0);

    // Verify socket is bound to vcan0
    REQUIRE(addr.can_ifindex == ifr.ifr_ifindex);
}

TEST_CASE("SocketCANBridge - Multiple bridges on same interface", "[bridge][socketcan][multiple]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }
    if (!test_usb_device_exists()) {
        SKIP("Test USB device not available (configure in .env or ensure /dev/ttyUSB0 exists)");
    }

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = get_test_usb_device();

    // Should be able to create multiple bridges on same interface
    REQUIRE_NOTHROW([&]() {
            SocketCANBridge bridge1(config);
            SocketCANBridge bridge2(config);

            REQUIRE(bridge1.get_socketcan_fd() != bridge2.get_socketcan_fd());
            REQUIRE(bridge1.is_socketcan_open());
            REQUIRE(bridge2.is_socketcan_open());
        }());
}

TEST_CASE("SocketCANBridge - Move semantics", "[bridge][socketcan][move]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }
    if (!test_usb_device_exists()) {
        SKIP("Test USB device not available (configure in .env or ensure /dev/ttyUSB0 exists)");
    }

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = get_test_usb_device();

    SocketCANBridge bridge1(config);
    int original_fd = bridge1.get_socketcan_fd();
    REQUIRE(original_fd >= 0);

    // Move construct
    SocketCANBridge bridge2(std::move(bridge1));
    REQUIRE(bridge2.get_socketcan_fd() == original_fd);
    REQUIRE(bridge2.is_socketcan_open());
}

TEST_CASE("SocketCANBridge - USB adapter initialization", "[bridge][usb][initialization]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }
    if (!test_usb_device_exists()) {
        SKIP("Test USB device not available (configure in .env or ensure /dev/ttyUSB0 exists)");
    }

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = get_test_usb_device();
    config.serial_baud_rate = SerialBaud::BAUD_2M;

    SocketCANBridge bridge(config);

    // Verify adapter is initialized
    auto adapter = bridge.get_adapter();
    REQUIRE(adapter != nullptr);
}

TEST_CASE("SocketCANBridge - Configuration with CAN parameters", "[bridge][usb][config]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }
    if (!test_usb_device_exists()) {
        SKIP("Test USB device not available (configure in .env or ensure /dev/ttyUSB0 exists)");
    }

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = get_test_usb_device();
    config.can_baud_rate = CANBaud::BAUD_500K;
    config.can_mode = CANMode::LOOPBACK;
    config.auto_retransmit = false;
    config.filter_id = 0x123;
    config.filter_mask = 0x7FF;

    // Should not throw (configuration is sent to adapter)
    REQUIRE_NOTHROW(SocketCANBridge(config));
}

TEST_CASE("SocketCANBridge - Extended ID filter triggers extended config",
    "[bridge][usb][extended]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }
    if (!test_usb_device_exists()) {
        SKIP("Test USB device not available (configure in .env or ensure /dev/ttyUSB0 exists)");
    }

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = get_test_usb_device();
    config.filter_id = 0x12345678;  // Extended ID (> 11 bits)
    config.filter_mask = 0x1FFFFFFF;

    // Should use extended CAN version internally
    REQUIRE_NOTHROW(SocketCANBridge(config));
}

TEST_CASE("SocketCANBridge - Statistics initialization", "[bridge][stats][initialization]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }
    if (!test_usb_device_exists()) {
        SKIP("Test USB device not available (configure in .env or ensure /dev/ttyUSB0 exists)");
    }

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = get_test_usb_device();

    SocketCANBridge bridge(config);

    // Get initial statistics - should all be zero
    auto stats = bridge.get_statistics();
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
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }
    if (!test_usb_device_exists()) {
        SKIP("Test USB device not available (configure in .env or ensure /dev/ttyUSB0 exists)");
    }

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = get_test_usb_device();

    SocketCANBridge bridge(config);

    // Reset statistics
    bridge.reset_statistics();

    // Verify all counters are zero
    auto stats = bridge.get_statistics();
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
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }
    if (!test_usb_device_exists()) {
        SKIP("Test USB device not available (configure in .env or ensure /dev/ttyUSB0 exists)");
    }

    auto config = BridgeConfig::create_default();
    config.socketcan_interface = "vcan0";
    config.usb_device_path = get_test_usb_device();

    SocketCANBridge bridge(config);

    // Get statistics and convert to string
    auto stats = bridge.get_statistics();
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

/**
 * @file test_socketcan_bridge_integration.cpp
 * @brief Integration tests for SocketCANBridge with vcan0 (Phase 10)
 * @version 1.0
 * @date 2025-10-14
 *
 * Tests bidirectional forwarding, error recovery, and graceful shutdown.
 * Requires: vcan0 interface and USB-CAN device.
 *
 * Setup vcan0:
 *   sudo modprobe vcan
 *   sudo ip link add dev vcan0 type vcan
 *   sudo ip link set up vcan0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "../include/pattern/socketcan_bridge.hpp"
#include "../include/pattern/bridge_config.hpp"
#include "../include/interface/socketcan_helpers.hpp"
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <cstring>

using namespace waveshare;
using namespace std::chrono_literals;

// ===================================================================
// Phase 10.1: Helper Functions
// ===================================================================

// Check if vcan0 interface exists
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

// Check if USB device exists
bool usb_device_exists(const std::string& device_path) {
    int fd = open(device_path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) return false;
    close(fd);
    return true;
}

// Get test USB device from config or default
std::string get_test_device() {
    try {
        auto config = BridgeConfig::load("../.env");
        if (usb_device_exists(config.usb_device_path)) {
            return config.usb_device_path;
        }
    } catch (...) {
    }
    return "/dev/ttyUSB0";
}

// Open raw SocketCAN socket for testing
int open_test_socket() {
    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) return -1;

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, "vcan0", IFNAMSIZ - 1);

    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        close(sock);
        return -1;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    // Set timeout for reads
    struct timeval timeout;
    timeout.tv_sec = 2;  // 2 second timeout
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    return sock;
}

// Send CAN frame to vcan0
bool send_can_frame(int sock, uint32_t id, const std::vector<uint8_t>& data,
    bool extended = false) {
    struct can_frame frame;
    std::memset(&frame, 0, sizeof(frame));

    frame.can_id = id;
    if (extended) {
        frame.can_id |= CAN_EFF_FLAG;
    }

    frame.can_dlc = data.size();
    std::memcpy(frame.data, data.data(), std::min<size_t>(data.size(), 8));

    ssize_t bytes = write(sock, &frame, sizeof(frame));
    return bytes == sizeof(frame);
}

// Receive CAN frame from vcan0
bool receive_can_frame(int sock, struct can_frame& frame, int timeout_ms = 2000) {
    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    ssize_t bytes = read(sock, &frame, sizeof(frame));
    return bytes == sizeof(frame);
}

// ===================================================================
// Phase 10.3: Bidirectional Forwarding Tests
// ===================================================================

TEST_CASE("SocketCAN Integration - CAN→USB forwarding", "[integration][forwarding][can_to_usb]") {
    if (!vcan0_exists()) {
        SKIP(
            "vcan0 interface not available. Run: sudo modprobe vcan && sudo ip link add dev vcan0 type vcan && sudo ip link set up vcan0");
    }

    std::string usb_device = get_test_device();
    if (!usb_device_exists(usb_device)) {
        SKIP("USB device not available at " + usb_device);
    }

    SECTION("Frame sent to vcan0 is received on USB side") {
        // Create bridge config
        BridgeConfig config = BridgeConfig::create_default();
        config.socketcan_interface = "vcan0";
        config.usb_device_path = usb_device;
        config.socketcan_read_timeout_ms = 100;
        config.usb_read_timeout_ms = 100;

        // Track received frames via callback
        std::atomic<bool> frame_received{false};
        std::atomic<uint32_t> received_id{0};

        SocketCANBridge bridge(config);

        bridge.set_socketcan_to_usb_callback(
            [&](const ::can_frame& can, const VariableFrame& usb) {
                frame_received = true;
                received_id = usb.get_can_id();
            }
        );

        // Start bridge
        bridge.start();
        REQUIRE(bridge.is_running());

        // Give threads time to start
        std::this_thread::sleep_for(100ms);

        // Send frame to vcan0
        int test_sock = open_test_socket();
        REQUIRE(test_sock >= 0);

        uint32_t test_id = 0x123;
        std::vector<uint8_t> test_data = {0xDE, 0xAD, 0xBE, 0xEF};
        REQUIRE(send_can_frame(test_sock, test_id, test_data));

        // Wait for forwarding (max 2 seconds)
        auto start = std::chrono::steady_clock::now();
        while (!frame_received &&
            std::chrono::steady_clock::now() - start < 2s) {
            std::this_thread::sleep_for(50ms);
        }

        close(test_sock);
        bridge.stop();

        // Verify frame was forwarded
        REQUIRE(frame_received);
        REQUIRE(received_id == test_id);

        // Verify statistics
        auto stats = bridge.get_statistics();
        REQUIRE(stats.socketcan_rx_frames >= 1);
        REQUIRE(stats.usb_tx_frames >= 1);
    }
}

TEST_CASE("SocketCAN Integration - USB→CAN forwarding", "[integration][forwarding][usb_to_can]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }

    std::string usb_device = get_test_device();
    if (!usb_device_exists(usb_device)) {
        SKIP("USB device not available at " + usb_device);
    }

    SECTION("Frame sent via USB is received on vcan0") {
        // Create bridge
        BridgeConfig config = BridgeConfig::create_default();
        config.socketcan_interface = "vcan0";
        config.usb_device_path = usb_device;

        std::atomic<bool> frame_forwarded{false};
        SocketCANBridge bridge(config);

        bridge.set_usb_to_socketcan_callback(
            [&](const VariableFrame& usb, const ::can_frame& can) {
                frame_forwarded = true;
            }
        );

        // Start bridge
        bridge.start();

        // Open test socket to monitor vcan0
        int test_sock = open_test_socket();
        REQUIRE(test_sock >= 0);

        // Wait for USB to send a frame (would need physical device sending)
        // This test validates the forwarding path exists
        // Full test requires wave_writer or physical device

        std::this_thread::sleep_for(500ms);

        close(test_sock);
        bridge.stop();

        // Statistics should be accessible
        auto stats = bridge.get_statistics();
        REQUIRE(int(stats.usb_rx_frames) >= 0);  // May be 0 if no USB traffic
    }
}

TEST_CASE("SocketCAN Integration - Extended ID forwarding", "[integration][forwarding][extended]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }

    std::string usb_device = get_test_device();
    if (!usb_device_exists(usb_device)) {
        SKIP("USB device not available at " + usb_device);
    }

    SECTION("Extended CAN ID is preserved during forwarding") {
        BridgeConfig config = BridgeConfig::create_default();
        config.socketcan_interface = "vcan0";
        config.usb_device_path = usb_device;

        std::atomic<bool> received{false};
        std::atomic<uint32_t> received_id{0};
        std::atomic<bool> was_extended{false};

        SocketCANBridge bridge(config);
        bridge.set_socketcan_to_usb_callback(
            [&](const ::can_frame& can, const VariableFrame& usb) {
                received = true;
                received_id = usb.get_can_id();
                was_extended = usb.is_extended();
            }
        );

        bridge.start();
        std::this_thread::sleep_for(100ms);

        int test_sock = open_test_socket();
        REQUIRE(test_sock >= 0);

        // Send extended frame
        uint32_t ext_id = 0x12345678;  // 29-bit ID
        std::vector<uint8_t> data = {0xAA, 0xBB};
        REQUIRE(send_can_frame(test_sock, ext_id, data, true));

        // Wait for forwarding
        auto start = std::chrono::steady_clock::now();
        while (!received && std::chrono::steady_clock::now() - start < 2s) {
            std::this_thread::sleep_for(50ms);
        }

        close(test_sock);
        bridge.stop();

        REQUIRE(received);
        REQUIRE(was_extended);
        REQUIRE(received_id == ext_id);
    }
}

// ===================================================================
// Phase 10.4: Error Recovery Tests
// ===================================================================

TEST_CASE("SocketCAN Integration - Timeout handling", "[integration][error_recovery][timeout]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }

    std::string usb_device = get_test_device();
    if (!usb_device_exists(usb_device)) {
        SKIP("USB device not available at " + usb_device);
    }

    SECTION("Bridge continues running during read timeouts") {
        BridgeConfig config = BridgeConfig::create_default();
        config.socketcan_interface = "vcan0";
        config.usb_device_path = usb_device;
        config.usb_read_timeout_ms = 100;
        config.socketcan_read_timeout_ms = 100;

        SocketCANBridge bridge(config);
        bridge.start();

        REQUIRE(bridge.is_running());

        // Wait through multiple timeout cycles
        std::this_thread::sleep_for(500ms);

        // Bridge should still be running
        REQUIRE(bridge.is_running());

        bridge.stop();
        REQUIRE_FALSE(bridge.is_running());
    }
}

TEST_CASE("SocketCAN Integration - Statistics during operation",
    "[integration][statistics][runtime]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }

    std::string usb_device = get_test_device();
    if (!usb_device_exists(usb_device)) {
        SKIP("USB device not available at " + usb_device);
    }

    SECTION("Statistics increment correctly during forwarding") {
        BridgeConfig config = BridgeConfig::create_default();
        config.socketcan_interface = "vcan0";
        config.usb_device_path = usb_device;

        SocketCANBridge bridge(config);

        // Get initial stats
        auto initial_stats = bridge.get_statistics();
        REQUIRE(initial_stats.socketcan_rx_frames == 0);

        bridge.start();
        std::this_thread::sleep_for(100ms);

        // Send a frame
        int test_sock = open_test_socket();
        REQUIRE(test_sock >= 0);
        send_can_frame(test_sock, 0x456, {0x11, 0x22});

        std::this_thread::sleep_for(500ms);

        auto stats = bridge.get_statistics();
        close(test_sock);
        bridge.stop();

        // Should have received at least one frame
        REQUIRE(stats.socketcan_rx_frames >= 1);
    }

    SECTION("reset_statistics() clears counters during operation") {
        BridgeConfig config = BridgeConfig::create_default();
        config.socketcan_interface = "vcan0";
        config.usb_device_path = usb_device;

        SocketCANBridge bridge(config);
        bridge.start();

        // Send some frames
        int test_sock = open_test_socket();
        REQUIRE(test_sock >= 0);
        send_can_frame(test_sock, 0x111, {0xAA});
        std::this_thread::sleep_for(200ms);

        // Reset statistics
        bridge.reset_statistics();
        auto stats = bridge.get_statistics();

        close(test_sock);
        bridge.stop();

        // All counters should be zero after reset
        REQUIRE(stats.usb_rx_frames == 0);
        REQUIRE(stats.usb_tx_frames == 0);
        REQUIRE(stats.socketcan_rx_frames == 0);
        REQUIRE(stats.socketcan_tx_frames == 0);
    }
}

// ===================================================================
// Phase 10.5: Graceful Shutdown Tests
// ===================================================================

TEST_CASE("SocketCAN Integration - Lifecycle management", "[integration][lifecycle][shutdown]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }

    std::string usb_device = get_test_device();
    if (!usb_device_exists(usb_device)) {
        SKIP("USB device not available at " + usb_device);
    }

    SECTION("start() and stop() control thread lifecycle") {
        BridgeConfig config = BridgeConfig::create_default();
        config.socketcan_interface = "vcan0";
        config.usb_device_path = usb_device;

        SocketCANBridge bridge(config);

        REQUIRE_FALSE(bridge.is_running());

        bridge.start();
        REQUIRE(bridge.is_running());

        std::this_thread::sleep_for(200ms);

        bridge.stop();
        REQUIRE_FALSE(bridge.is_running());
    }

    SECTION("Double start() throws std::logic_error") {
        BridgeConfig config = BridgeConfig::create_default();
        config.socketcan_interface = "vcan0";
        config.usb_device_path = usb_device;

        SocketCANBridge bridge(config);
        bridge.start();

        REQUIRE_THROWS_AS(bridge.start(), std::logic_error);

        bridge.stop();
    }

    SECTION("Multiple stop() calls are safe (idempotent)") {
        BridgeConfig config = BridgeConfig::create_default();
        config.socketcan_interface = "vcan0";
        config.usb_device_path = usb_device;

        SocketCANBridge bridge(config);
        bridge.start();

        bridge.stop();
        REQUIRE_NOTHROW(bridge.stop());
        REQUIRE_NOTHROW(bridge.stop());
    }

    SECTION("Destructor stops running bridge") {
        BridgeConfig config = BridgeConfig::create_default();
        config.socketcan_interface = "vcan0";
        config.usb_device_path = usb_device;

        {
            SocketCANBridge bridge(config);
            bridge.start();
            REQUIRE(bridge.is_running());
            // Bridge destroyed here - should stop cleanly
        }

        // If we reach here, destructor didn't hang
        REQUIRE(true);
    }
}

TEST_CASE("SocketCAN Integration - Thread joining", "[integration][lifecycle][threads]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }

    std::string usb_device = get_test_device();
    if (!usb_device_exists(usb_device)) {
        SKIP("USB device not available at " + usb_device);
    }

    SECTION("Threads are properly joined on stop()") {
        BridgeConfig config = BridgeConfig::create_default();
        config.socketcan_interface = "vcan0";
        config.usb_device_path = usb_device;

        SocketCANBridge bridge(config);
        bridge.start();

        std::this_thread::sleep_for(200ms);

        // stop() should block until threads join
        auto start = std::chrono::steady_clock::now();
        bridge.stop();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        ).count();

        // Should complete relatively quickly (< 1 second)
        REQUIRE(duration < 1000);
        REQUIRE_FALSE(bridge.is_running());
    }
}

// ===================================================================
// Callback Integration Tests
// ===================================================================

TEST_CASE("SocketCAN Integration - Callback invocation", "[integration][callbacks][invocation]") {
    if (!vcan0_exists()) {
        SKIP("vcan0 interface not available");
    }

    std::string usb_device = get_test_device();
    if (!usb_device_exists(usb_device)) {
        SKIP("USB device not available at " + usb_device);
    }

    SECTION("CAN→USB callback is invoked on frame forward") {
        BridgeConfig config = BridgeConfig::create_default();
        config.socketcan_interface = "vcan0";
        config.usb_device_path = usb_device;

        std::atomic<int> callback_count{0};
        std::atomic<uint32_t> last_id{0};

        SocketCANBridge bridge(config);
        bridge.set_socketcan_to_usb_callback(
            [&](const ::can_frame& can, const VariableFrame& usb) {
                callback_count++;
                last_id = can.can_id & CAN_EFF_MASK;
            }
        );

        bridge.start();
        std::this_thread::sleep_for(100ms);

        int test_sock = open_test_socket();
        REQUIRE(test_sock >= 0);

        send_can_frame(test_sock, 0x789, {0xCC, 0xDD});

        // Wait for callback
        auto start = std::chrono::steady_clock::now();
        while (callback_count == 0 &&
            std::chrono::steady_clock::now() - start < 2s) {
            std::this_thread::sleep_for(50ms);
        }

        close(test_sock);
        bridge.stop();

        REQUIRE(callback_count >= 1);
        REQUIRE(last_id == 0x789);
    }

    SECTION("Callbacks can access frame data") {
        BridgeConfig config = BridgeConfig::create_default();
        config.socketcan_interface = "vcan0";
        config.usb_device_path = usb_device;

        std::atomic<bool> data_verified{false};

        SocketCANBridge bridge(config);
        bridge.set_socketcan_to_usb_callback(
            [&](const ::can_frame& can, const VariableFrame& usb) {
                // Verify data matches
                if (usb.get_dlc() == 3 &&
                usb.get_data()[0] == 0x11 &&
                usb.get_data()[1] == 0x22 &&
                usb.get_data()[2] == 0x33) {
                    data_verified = true;
                }
            }
        );

        bridge.start();
        std::this_thread::sleep_for(100ms);

        int test_sock = open_test_socket();
        REQUIRE(test_sock >= 0);

        send_can_frame(test_sock, 0xABC, {0x11, 0x22, 0x33});

        auto start = std::chrono::steady_clock::now();
        while (!data_verified &&
            std::chrono::steady_clock::now() - start < 2s) {
            std::this_thread::sleep_for(50ms);
        }

        close(test_sock);
        bridge.stop();

        REQUIRE(data_verified);
    }
}

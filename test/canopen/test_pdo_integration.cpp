/**
 * @file test_pdo_integration.cpp
 * @brief Integration tests for PDO communication with mock motor driver
 * @author effibot (andrea.efficace1@gmail.com)
 * @date 2025-11-09
 *
 * This test requires:
 * 1. vcan_test interface up: sudo ip link add dev vcan_test type vcan && sudo ip link set vcan_test up
 * 2. MockMotorResponder with PDO support (automatically started by fixture)
 *
 * Note: Uses vcan_test instead of vcan0 to avoid conflicts with running bridges/motors.
 *
 * Usage:
 *   ./build/test/test_pdo_integration
 *
 * This test will:
 * - Test TPDO1 reception (mock motor sends position/statusword feedback)
 * - Test TPDO2 reception (mock motor sends velocity feedback)
 * - Test RPDO1 sending (host sends controlword/target position commands)
 * - Test RPDO2 sending (host sends additional commands)
 * - Test SYNC message triggering of PDOs
 * - Test PDO mapping and configuration
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "canopen/pdo_manager.hpp"
#include "canopen/pdo_constants.hpp"
#include "canopen/cia402_constants.hpp"
#include "test_utils_canopen.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace canopen;
using namespace canopen::pdo;
using namespace canopen::cia402;
using namespace test_utils;
using Catch::Matchers::ContainsSubstring;

// ==============================================================================
// HELPER FUNCTIONS
// ==============================================================================

/**
 * @brief Check if vcan_test interface is available
 */
bool is_vcan_test_available() {
    return std::filesystem::exists("/sys/class/net/vcan_test");
}

/**
 * @brief Enhanced mock motor with PDO support
 */
class PDOMockMotor {
    public:
        explicit PDOMockMotor(uint8_t node_id = 127, const std::string& interface = "vcan_test")
            : node_id_(node_id)
            , running_(false)
            , position_actual_(0)
            , velocity_actual_(0)
            , statusword_(0x0040) // SWITCH_ON_DISABLED
            , controlword_(0x0000) {
            try {
                socket_ = create_test_socket(interface);
            } catch (const std::exception& e) {
                std::cerr << "[PDOMockMotor] Failed to create socket: " << e.what() << std::endl;
                return;
            }
        }

        ~PDOMockMotor() {
            stop();
        }

        void start() {
            if (!socket_ || !socket_->is_open()) {
                std::cerr << "[PDOMockMotor] Cannot start: socket not open" << std::endl;
                return;
            }

            if (running_.load()) return;

            running_.store(true);
            responder_thread_ = std::thread(&PDOMockMotor::pdo_loop, this);

            std::cout << "[PDOMockMotor] Started PDO responder for node "
                      << static_cast<int>(node_id_) << " on "
                      << socket_->get_interface_name() << std::endl;
        }

        void stop() {
            if (!running_.load()) return;

            running_.store(false);
            if (responder_thread_.joinable()) {
                responder_thread_.join();
            }

            std::cout << "[PDOMockMotor] Stopped PDO responder for node "
                      << static_cast<int>(node_id_) << std::endl;
        }

        bool is_running() const { return running_.load(); }

        // Set mock motor state for testing
        void set_position(int32_t position) { position_actual_.store(position); }
        void set_velocity(int32_t velocity) { velocity_actual_.store(velocity); }
        void set_statusword(uint16_t statusword) { statusword_.store(statusword); }

        // Get received commands
        uint16_t get_received_controlword() const { return controlword_.load(); }
        int32_t get_received_target_position() const { return target_position_.load(); }

        // Manually send TPDO (for testing)
        void send_tpdo1() {
            struct can_frame frame;
            std::memset(&frame, 0, sizeof(frame));
            frame.can_id = cob_id::TPDO1_BASE + node_id_;
            frame.can_dlc = 8;

            // TPDO1: Statusword (2 bytes) + Position Actual (4 bytes)
            uint16_t sw = statusword_.load();
            int32_t pos = position_actual_.load();

            frame.data[0] = sw & 0xFF;
            frame.data[1] = (sw >> 8) & 0xFF;
            frame.data[2] = pos & 0xFF;
            frame.data[3] = (pos >> 8) & 0xFF;
            frame.data[4] = (pos >> 16) & 0xFF;
            frame.data[5] = (pos >> 24) & 0xFF;
            frame.data[6] = 0x00; // Reserved
            frame.data[7] = 0x00; // Reserved

            if (socket_ && socket_->is_open()) {
                socket_->send(frame);
            }
        }

        void send_tpdo2() {
            struct can_frame frame;
            std::memset(&frame, 0, sizeof(frame));
            frame.can_id = cob_id::TPDO2_BASE + node_id_;
            frame.can_dlc = 8;

            // TPDO2: Velocity Actual (4 bytes) + Current Actual (2 bytes)
            int32_t vel = velocity_actual_.load();
            uint16_t current = 0; // Mock current value

            frame.data[0] = vel & 0xFF;
            frame.data[1] = (vel >> 8) & 0xFF;
            frame.data[2] = (vel >> 16) & 0xFF;
            frame.data[3] = (vel >> 24) & 0xFF;
            frame.data[4] = current & 0xFF;
            frame.data[5] = (current >> 8) & 0xFF;
            frame.data[6] = 0x00; // Reserved
            frame.data[7] = 0x00; // Reserved

            if (socket_ && socket_->is_open()) {
                socket_->send(frame);
            }
        }

    private:
        void pdo_loop() {
            struct can_frame rx_frame;
            const uint32_t rpdo1_cob = cob_id::RPDO1_BASE + node_id_;
            const uint32_t rpdo2_cob = cob_id::RPDO2_BASE + node_id_;
            const uint32_t sync_cob = 0x80; // SYNC message

            while (running_.load()) {
                // Wait for frame with timeout
                fd_set readfds;
                FD_ZERO(&readfds);
                FD_SET(socket_->get_fd(), &readfds);

                struct timeval timeout;
                timeout.tv_sec = 0;
                timeout.tv_usec = 10000; // 10ms

                int ret = select(socket_->get_fd() + 1, &readfds, nullptr, nullptr, &timeout);

                if (ret <= 0) continue; // Timeout or error

                // Read frame
                ssize_t nbytes = socket_->receive(rx_frame);
                if (nbytes != sizeof(rx_frame)) continue;

                // Handle different PDO types
                if (rx_frame.can_id == rpdo1_cob) {
                    // RPDO1: Controlword (2 bytes) + Target Position (4 bytes)
                    if (rx_frame.can_dlc >= 6) {
                        uint16_t cw = rx_frame.data[0] | (rx_frame.data[1] << 8);
                        int32_t target = rx_frame.data[2] | (rx_frame.data[3] << 8) |
                            (rx_frame.data[4] << 16) | (rx_frame.data[5] << 24);

                        controlword_.store(cw);
                        target_position_.store(target);

                        std::cout << "[PDOMockMotor] RPDO1 received: CW=0x" << std::hex << cw
                                  << " Target=" << std::dec << target << std::endl;
                    }
                } else if (rx_frame.can_id == rpdo2_cob) {
                    // RPDO2: Additional parameters
                    std::cout << "[PDOMockMotor] RPDO2 received" << std::endl;
                } else if (rx_frame.can_id == sync_cob) {
                    // SYNC: Trigger TPDO transmission
                    std::cout << "[PDOMockMotor] SYNC received - sending TPDOs" << std::endl;
                    send_tpdo1();
                    send_tpdo2();
                }
            }
        }

        uint8_t node_id_;
        std::atomic<bool> running_;
        std::shared_ptr<waveshare::ICANSocket> socket_;
        std::thread responder_thread_;

        std::atomic<int32_t> position_actual_;
        std::atomic<int32_t> velocity_actual_;
        std::atomic<uint16_t> statusword_;
        std::atomic<uint16_t> controlword_;
        std::atomic<int32_t> target_position_;
};

/**
 * @brief RAII fixture for PDO integration tests
 */
class PDOIntegrationFixture {
    protected:
        PDOIntegrationFixture(uint8_t node_id = 127)
            : pdo_motor_(node_id, "vcan_test") {
            if (is_vcan_test_available()) {
                pdo_motor_.start();
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Let it initialize
            }
        }

        ~PDOIntegrationFixture() {
            pdo_motor_.stop();
        }

        PDOMockMotor pdo_motor_;
};

// ==============================================================================
// INTEGRATION TESTS
// ==============================================================================

TEST_CASE("PDO Integration: Environment Check", "[integration][pdo][setup]") {
    SECTION("vcan_test interface availability") {
        if (!is_vcan_test_available()) {
            WARN(
                "vcan_test interface not found. Run: sudo ip link add dev vcan_test type vcan && sudo ip link set vcan_test up");
            SKIP("vcan_test not available - skipping integration tests");
        }
        REQUIRE(is_vcan_test_available());
        INFO("✓ vcan_test interface is available");
    }
}

TEST_CASE_METHOD(PDOIntegrationFixture, "PDO Integration: TPDO1 Reception",
    "[integration][pdo][tpdo1]") {
    if (!is_vcan_test_available()) {
        SKIP("vcan_test not available");
    }

    INFO("=== Testing TPDO1 Reception (Statusword + Position) ===");

    try {
        // Create PDO manager
        auto socket = create_test_socket("vcan_test");
        PDOManager pdo_mgr(socket);

        // Register TPDO1 callback
        std::atomic<bool> tpdo1_received{false};
        std::atomic<uint16_t> received_statusword{0};
        std::atomic<int32_t> received_position{0};

        pdo_mgr.register_tpdo1_callback(127, [&](const can_frame& frame) {
                // Parse TPDO1: Statusword (2 bytes) + Position (4 bytes)
                if (frame.can_dlc >= 6) {
                    uint16_t sw = frame.data[0] | (frame.data[1] << 8);
                    int32_t pos = frame.data[2] | (frame.data[3] << 8) |
                    (frame.data[4] << 16) | (frame.data[5] << 24);

                    received_statusword.store(sw);
                    received_position.store(pos);
                    tpdo1_received.store(true);

                    INFO("  TPDO1 received: SW=0x" << std::hex << sw
                                                   << " Pos=" << std::dec << pos);
                }
            });

        // Start PDO manager
        REQUIRE(pdo_mgr.start());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Set mock motor values
        pdo_motor_.set_statusword(0x0637);  // OPERATION_ENABLED state
        pdo_motor_.set_position(12345);

        // Trigger TPDO1 transmission
        pdo_motor_.send_tpdo1();

        // Wait for callback
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Verify reception
        REQUIRE(tpdo1_received.load());
        REQUIRE(received_statusword.load() == 0x0637);
        REQUIRE(received_position.load() == 12345);

        INFO("✓ TPDO1 successfully received and parsed");

        pdo_mgr.stop();
    } catch (const std::exception& e) {
        FAIL("TPDO1 test failed: " << e.what());
    }
}

TEST_CASE_METHOD(PDOIntegrationFixture, "PDO Integration: TPDO2 Reception",
    "[integration][pdo][tpdo2]") {
    if (!is_vcan_test_available()) {
        SKIP("vcan_test not available");
    }

    INFO("=== Testing TPDO2 Reception (Velocity) ===");

    try {
        auto socket = create_test_socket("vcan_test");
        PDOManager pdo_mgr(socket);

        // Register TPDO2 callback
        std::atomic<bool> tpdo2_received{false};
        std::atomic<int32_t> received_velocity{0};

        pdo_mgr.register_tpdo2_callback(127, [&](const can_frame& frame) {
                // Parse TPDO2: Velocity (4 bytes) + Current (2 bytes)
                if (frame.can_dlc >= 4) {
                    int32_t vel = frame.data[0] | (frame.data[1] << 8) |
                    (frame.data[2] << 16) | (frame.data[3] << 24);

                    received_velocity.store(vel);
                    tpdo2_received.store(true);

                    INFO("  TPDO2 received: Vel=" << std::dec << vel);
                }
            });

        REQUIRE(pdo_mgr.start());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Set mock motor velocity
        pdo_motor_.set_velocity(5000);

        // Trigger TPDO2 transmission
        pdo_motor_.send_tpdo2();

        // Wait for callback
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Verify reception
        REQUIRE(tpdo2_received.load());
        REQUIRE(received_velocity.load() == 5000);

        INFO("✓ TPDO2 successfully received and parsed");

        pdo_mgr.stop();
    } catch (const std::exception& e) {
        FAIL("TPDO2 test failed: " << e.what());
    }
}

TEST_CASE_METHOD(PDOIntegrationFixture, "PDO Integration: RPDO1 Sending",
    "[integration][pdo][rpdo1]") {
    if (!is_vcan_test_available()) {
        SKIP("vcan_test not available");
    }

    INFO("=== Testing RPDO1 Sending (Controlword + Target Position) ===");

    try {
        auto socket = create_test_socket("vcan_test");
        PDOManager pdo_mgr(socket);

        REQUIRE(pdo_mgr.start());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Prepare RPDO1 data: Controlword (2 bytes) + Target Position (4 bytes)
        uint16_t controlword = 0x000F;  // Enable Operation command
        int32_t target_position = 98765;

        std::vector<uint8_t> rpdo1_data = {
            static_cast<uint8_t>(controlword & 0xFF),
            static_cast<uint8_t>((controlword >> 8) & 0xFF),
            static_cast<uint8_t>(target_position & 0xFF),
            static_cast<uint8_t>((target_position >> 8) & 0xFF),
            static_cast<uint8_t>((target_position >> 16) & 0xFF),
            static_cast<uint8_t>((target_position >> 24) & 0xFF)
        };

        // Send RPDO1
        INFO("Sending RPDO1: CW=0x" << std::hex << controlword
                                    << " Target=" << std::dec << target_position);

        REQUIRE(pdo_mgr.send_rpdo1(127, rpdo1_data));

        // Wait for mock motor to receive
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Verify mock motor received the data
        REQUIRE(pdo_motor_.get_received_controlword() == controlword);
        REQUIRE(pdo_motor_.get_received_target_position() == target_position);

        INFO("✓ RPDO1 successfully sent and received by motor");

        pdo_mgr.stop();
    } catch (const std::exception& e) {
        FAIL("RPDO1 test failed: " << e.what());
    }
}

TEST_CASE_METHOD(PDOIntegrationFixture, "PDO Integration: RPDO2 Sending",
    "[integration][pdo][rpdo2]") {
    if (!is_vcan_test_available()) {
        SKIP("vcan_test not available");
    }

    INFO("=== Testing RPDO2 Sending ===");

    try {
        auto socket = create_test_socket("vcan_test");
        PDOManager pdo_mgr(socket);

        REQUIRE(pdo_mgr.start());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Prepare RPDO2 data
        std::vector<uint8_t> rpdo2_data = {0x01, 0x02, 0x03, 0x04};

        // Send RPDO2
        INFO("Sending RPDO2 with " << rpdo2_data.size() << " bytes");
        REQUIRE(pdo_mgr.send_rpdo2(127, rpdo2_data));

        INFO("✓ RPDO2 successfully sent");

        pdo_mgr.stop();
    } catch (const std::exception& e) {
        FAIL("RPDO2 test failed: " << e.what());
    }
}

TEST_CASE_METHOD(PDOIntegrationFixture, "PDO Integration: SYNC Triggered TPDOs",
    "[integration][pdo][sync]") {
    if (!is_vcan_test_available()) {
        SKIP("vcan_test not available");
    }

    INFO("=== Testing SYNC-Triggered TPDO Transmission ===");

    try {
        auto socket = create_test_socket("vcan_test");
        PDOManager pdo_mgr(socket);

        // Register callbacks for both TPDOs
        std::atomic<bool> tpdo1_received{false};
        std::atomic<bool> tpdo2_received{false};

        pdo_mgr.register_tpdo1_callback(127, [&](const can_frame&) {
                tpdo1_received.store(true);
                INFO("  TPDO1 received after SYNC");
            });

        pdo_mgr.register_tpdo2_callback(127, [&](const can_frame&) {
                tpdo2_received.store(true);
                INFO("  TPDO2 received after SYNC");
            });

        REQUIRE(pdo_mgr.start());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Set mock motor values
        pdo_motor_.set_statusword(0x0237);
        pdo_motor_.set_position(11111);
        pdo_motor_.set_velocity(2222);

        // Send SYNC message manually
        struct can_frame sync_frame;
        std::memset(&sync_frame, 0, sizeof(sync_frame));
        sync_frame.can_id = 0x80;  // SYNC COB-ID
        sync_frame.can_dlc = 0;

        INFO("Sending SYNC message (COB-ID 0x80)");
        socket->send(sync_frame);

        // Wait for TPDOs to be transmitted in response to SYNC
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        // Verify both TPDOs were received
        REQUIRE(tpdo1_received.load());
        REQUIRE(tpdo2_received.load());

        INFO("✓ Both TPDOs successfully triggered by SYNC");

        pdo_mgr.stop();
    } catch (const std::exception& e) {
        FAIL("SYNC test failed: " << e.what());
    }
}

TEST_CASE_METHOD(PDOIntegrationFixture, "PDO Integration: Multi-Motor Support",
    "[integration][pdo][multi_motor]") {
    if (!is_vcan_test_available()) {
        SKIP("vcan_test not available");
    }

    INFO("=== Testing Multi-Motor PDO Support ===");

    try {
        auto socket = create_test_socket("vcan_test");
        PDOManager pdo_mgr(socket);

        // Register callbacks for multiple motors
        std::atomic<int> callback_count{0};

        for (uint8_t node_id = 1; node_id <= 4; ++node_id) {
            pdo_mgr.register_tpdo1_callback(node_id, [&, node_id](const can_frame&) {
                    callback_count++;
                    INFO("  TPDO1 received from motor " << static_cast<int>(node_id));
                });
        }

        REQUIRE(pdo_mgr.start());

        // Test sending to different motors
        std::vector<uint8_t> test_data = {0x01, 0x02};

        REQUIRE(pdo_mgr.send_rpdo1(1, test_data));
        REQUIRE(pdo_mgr.send_rpdo1(2, test_data));
        REQUIRE(pdo_mgr.send_rpdo1(3, test_data));
        REQUIRE(pdo_mgr.send_rpdo1(4, test_data));

        INFO("✓ Successfully registered and sent PDOs for 4 motors");

        pdo_mgr.stop();
    } catch (const std::exception& e) {
        FAIL("Multi-motor test failed: " << e.what());
    }
}

TEST_CASE_METHOD(PDOIntegrationFixture, "PDO Integration: Statistics Tracking",
    "[integration][pdo][statistics]") {
    if (!is_vcan_test_available()) {
        SKIP("vcan_test not available");
    }

    INFO("=== Testing PDO Statistics ===");

    try {
        auto socket = create_test_socket("vcan_test");
        PDOManager pdo_mgr(socket);

        pdo_mgr.register_tpdo1_callback(127, [](const can_frame&) {
            });

        REQUIRE(pdo_mgr.start());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Send some PDOs
        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
        pdo_mgr.send_rpdo1(127, data);
        pdo_mgr.send_rpdo1(127, data);

        // Trigger TPDOs
        pdo_motor_.send_tpdo1();
        pdo_motor_.send_tpdo1();
        pdo_motor_.send_tpdo1();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Get statistics
        auto stats = pdo_mgr.get_statistics(127);

        INFO("  RPDO1 sent: " << stats.rpdo1_sent);
        INFO("  TPDO1 received: " << stats.tpdo1_received);

        REQUIRE(stats.rpdo1_sent >= 2);
        REQUIRE(stats.tpdo1_received >= 3);

        INFO("✓ Statistics successfully tracked");

        pdo_mgr.stop();
    } catch (const std::exception& e) {
        FAIL("Statistics test failed: " << e.what());
    }
}

TEST_CASE("PDO Integration: COB-ID Calculation", "[integration][pdo][cob_id]") {
    INFO("=== Testing COB-ID Calculations ===");

    // Test standard COB-ID calculations for node 127
    uint8_t node_id = 127;

    uint32_t rpdo1_cob = cob_id::RPDO1_BASE + node_id;
    uint32_t rpdo2_cob = cob_id::RPDO2_BASE + node_id;
    uint32_t tpdo1_cob = cob_id::TPDO1_BASE + node_id;
    uint32_t tpdo2_cob = cob_id::TPDO2_BASE + node_id;

    INFO("  RPDO1 COB-ID: 0x" << std::hex << rpdo1_cob);
    INFO("  RPDO2 COB-ID: 0x" << std::hex << rpdo2_cob);
    INFO("  TPDO1 COB-ID: 0x" << std::hex << tpdo1_cob);
    INFO("  TPDO2 COB-ID: 0x" << std::hex << tpdo2_cob);

    REQUIRE(rpdo1_cob == 0x27F);  // 0x200 + 127
    REQUIRE(rpdo2_cob == 0x37F);  // 0x300 + 127
    REQUIRE(tpdo1_cob == 0x1FF);  // 0x180 + 127
    REQUIRE(tpdo2_cob == 0x2FF);  // 0x280 + 127

    INFO("✓ COB-ID calculations correct");
}

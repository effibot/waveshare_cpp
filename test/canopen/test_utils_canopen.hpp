/**
 * @file test_utils_canopen.hpp
 * @brief Shared test utilities for CANopen tests
 * @author effibot (andrea.efficace1@gmail.com)
 * @date 2025-11-07
 */

#pragma once

#include "io/can_socket.hpp"
#include "io/real_can_socket.hpp"
#include "canopen/object_dictionary.hpp"
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <linux/can.h>
#include <sys/select.h>
#include <cstring>
#include <iostream>

namespace test_utils {

/**
 * @brief Create a real CAN socket for testing
 * @param interface_name CAN interface name (default: "vcan0")
 * @param timeout_ms Receive timeout in milliseconds (default: 1000ms)
 * @return Shared pointer to ICANSocket (RealCANSocket implementation)
 *
 * This helper function is automatically available to all CANopen tests
 * via CMake target_include_directories configuration.
 */
    inline std::shared_ptr<waveshare::ICANSocket> create_test_socket(
        const std::string& interface_name = "vcan0",
        int timeout_ms = 1000
    ) {
        return std::make_shared<waveshare::RealCANSocket>(interface_name, timeout_ms);
    }

/**
 * @brief Load test object dictionary from config file
 * @return ObjectDictionary loaded from motor_config.json
 *
 * Automatically resolves the config file path relative to the build directory.
 */
    inline canopen::ObjectDictionary load_test_dictionary() {
        // Path relative to build/test directory
        return canopen::ObjectDictionary("../../config/motor_config.json");
    }

/**
 * @brief Mock CANopen motor responder for integration tests
 *
 * Automatically responds to SDO requests on vcan0 with realistic motor data.
 * Runs in background thread to simulate a real motor driver.
 */
    class MockMotorResponder {
        public:
            explicit MockMotorResponder(uint8_t node_id, const std::string& interface = "vcan0")
                : node_id_(node_id)
                , running_(false) {
                try {
                    socket_ = create_test_socket(interface);
                } catch (const std::exception& e) {
                    std::cerr << "[MockMotor] Failed to create socket: " << e.what() << std::endl;
                    return;
                }

                // Initialize mock register values (CIA402 standard registers)
                registers_[0x6041] = 0x0637; // Statusword: ready to switch on
                registers_[0x1001] = 0x00; // Error register: no error
                registers_[0x6061] = 0x01; // Mode of operation display: Profile Position
                registers_[0x6064] = 0; // Position actual value
                registers_[0x606C] = 0; // Velocity actual value
                registers_[0x6077] = 0; // Torque actual value
            }

            ~MockMotorResponder() {
                stop();
            }

            void start() {
                if (!socket_ || !socket_->is_open()) {
                    std::cerr << "[MockMotor] Cannot start: socket not open" << std::endl;
                    return;
                }

                if (running_.load()) return;

                running_.store(true);
                responder_thread_ = std::thread(&MockMotorResponder::response_loop, this);

                std::cout << "[MockMotor] Started responder for node "
                          << static_cast<int>(node_id_) << " on "
                          << socket_->get_interface_name() << std::endl;
            }

            void stop() {
                if (!running_.load()) return;

                running_.store(false);
                if (responder_thread_.joinable()) {
                    responder_thread_.join();
                }

                std::cout << "[MockMotor] Stopped responder for node "
                          << static_cast<int>(node_id_) << std::endl;
            }

            bool is_running() const { return running_.load(); }

            // Set mock register values for testing
            void set_register(uint16_t index, uint32_t value) {
                std::lock_guard<std::mutex> lock(registers_mutex_);
                registers_[index] = value;
            }

            uint32_t get_register(uint16_t index) const {
                std::lock_guard<std::mutex> lock(registers_mutex_);
                auto it = registers_.find(index);
                return (it != registers_.end()) ? it->second : 0;
            }

        private:
            void response_loop() {
                struct can_frame rx_frame, tx_frame;
                const uint32_t sdo_rx_cob = 0x600 + node_id_; // SDO from client
                const uint32_t sdo_tx_cob = 0x580 + node_id_; // SDO to client

                while (running_.load()) {
                    // Wait for frame with timeout
                    fd_set readfds;
                    FD_ZERO(&readfds);
                    FD_SET(socket_->get_fd(), &readfds);

                    struct timeval timeout;
                    timeout.tv_sec = 0;
                    timeout.tv_usec = 100000; // 100ms

                    int ret = select(socket_->get_fd() + 1, &readfds, nullptr, nullptr, &timeout);

                    if (ret <= 0) continue; // Timeout or error

                    // Read frame
                    ssize_t nbytes = socket_->receive(rx_frame);
                    if (nbytes != sizeof(rx_frame)) continue;

                    // Check if it's for us
                    if (rx_frame.can_id != sdo_rx_cob) continue;

                    // Parse SDO command
                    uint8_t cmd = rx_frame.data[0];
                    uint16_t index = rx_frame.data[1] | (rx_frame.data[2] << 8);
                    uint8_t subindex = rx_frame.data[3];

                    std::memset(&tx_frame, 0, sizeof(tx_frame));
                    tx_frame.can_id = sdo_tx_cob;
                    tx_frame.can_dlc = 8;

                    if (cmd == 0x40) {
                        // Upload request (read) - client wants to read from motor
                        std::lock_guard<std::mutex> lock(registers_mutex_);
                        auto it = registers_.find(index);

                        if (it != registers_.end()) {
                            // Send expedited upload response (4 bytes)
                            tx_frame.data[0] = 0x43; // Upload response, 4 bytes
                            tx_frame.data[1] = index & 0xFF;
                            tx_frame.data[2] = (index >> 8) & 0xFF;
                            tx_frame.data[3] = subindex;

                            uint32_t value = it->second;
                            tx_frame.data[4] = value & 0xFF;
                            tx_frame.data[5] = (value >> 8) & 0xFF;
                            tx_frame.data[6] = (value >> 16) & 0xFF;
                            tx_frame.data[7] = (value >> 24) & 0xFF;
                        } else {
                            // Object not found - send abort
                            tx_frame.data[0] = 0x80;
                            tx_frame.data[1] = index & 0xFF;
                            tx_frame.data[2] = (index >> 8) & 0xFF;
                            tx_frame.data[3] = subindex;
                            tx_frame.data[4] = 0x11; // Abort code: object does not exist
                            tx_frame.data[5] = 0x00;
                            tx_frame.data[6] = 0x02;
                            tx_frame.data[7] = 0x06;
                        }
                    } else if ((cmd & 0xE0) == 0x20) {
                        // Download request (write) - client wants to write to motor
                        uint32_t value = rx_frame.data[4] | (rx_frame.data[5] << 8) |
                            (rx_frame.data[6] << 16) | (rx_frame.data[7] << 24);

                        {
                            std::lock_guard<std::mutex> lock(registers_mutex_);
                            registers_[index] = value;

                            // Special handling: mirror mode of operation write (0x6060) to display (0x6061)
                            if (index == 0x6060) {
                                registers_[0x6061] = value;
                            }
                        }

                        // Send download response (write confirmation)
                        tx_frame.data[0] = 0x60;
                        tx_frame.data[1] = index & 0xFF;
                        tx_frame.data[2] = (index >> 8) & 0xFF;
                        tx_frame.data[3] = subindex;
                    }

                    // Send response
                    socket_->send(tx_frame);
                }
            }

            uint8_t node_id_;
            std::atomic<bool> running_;
            std::shared_ptr<waveshare::ICANSocket> socket_;
            std::thread responder_thread_;

            mutable std::mutex registers_mutex_;
            std::map<uint16_t, uint32_t> registers_;
    };

/**
 * @brief RAII fixture for CANopen integration tests with automatic mock motor
 *
 * Usage with Catch2:
 *
 * TEST_CASE_METHOD(CANopenIntegrationFixture, "My test", "[integration]") {
 *     // mock_motor is already running on vcan0
 *     auto socket = create_test_socket("vcan0");
 *     auto dict = load_test_dictionary();
 *     SDOClient client(socket, dict, 1);
 *     // ... test code ...
 * }
 */
    class CANopenIntegrationFixture {
        protected:
            CANopenIntegrationFixture(uint8_t node_id = 1)
                : mock_motor(node_id, "vcan0") {
                if (is_vcan_available()) {
                    mock_motor.start();
                    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Let it initialize
                }
            }

            ~CANopenIntegrationFixture() {
                mock_motor.stop();
            }

            static bool is_vcan_available() {
                try {
                    auto test_socket = create_test_socket("vcan0");
                    return test_socket && test_socket->is_open();
                } catch (...) {
                    return false;
                }
            }

            MockMotorResponder mock_motor;
    };

} // namespace test_utils

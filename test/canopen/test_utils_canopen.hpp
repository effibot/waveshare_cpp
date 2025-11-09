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
#include "canopen/cia402_constants.hpp"
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
 * @brief Create a CAN socket for testing
 * @param interface_name CAN interface name (default: "vcan_test")
 * @param timeout_ms Socket receive timeout in milliseconds
 * @return Shared pointer to ICANSocket (RealCANSocket implementation)
 *
 * This helper function is automatically available to all CANopen tests
 * via CMake target_include_directories configuration.
 */
    inline std::shared_ptr<waveshare::ICANSocket> create_test_socket(
        const std::string& interface_name = "vcan_test",
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
 * Automatically responds to SDO requests on vcan_test with realistic motor data.
 * Runs in background thread to simulate a real motor driver with full CIA402 FSM.
 *
 * Uses node ID 127 (maximum for standard 11-bit CAN IDs) to avoid conflicts with
 * real motor drivers which typically use IDs 1-10.
 */
    class MockMotorResponder {
        public:
            explicit MockMotorResponder(uint8_t node_id = 127,
                const std::string& interface = "vcan_test")
                : node_id_(node_id)
                , running_(false)
                , current_state_(canopen::cia402::State::SWITCH_ON_DISABLED) {
                try {
                    socket_ = create_test_socket(interface);
                } catch (const std::exception& e) {
                    std::cerr << "[MockMotor] Failed to create socket: " << e.what() << std::endl;
                    return;
                }

                // Initialize mock register values (CIA402 standard registers)
                update_statusword_from_state();
                registers_[0x1001] = 0x00; // Error register: no error
                registers_[0x6061] = 0x01; // Mode of operation display: Profile Position
                registers_[0x6064] = 0; // Position actual value
                registers_[0x606C] = 0; // Velocity actual value
                registers_[0x6077] = 0; // Torque actual value
                registers_[0x6040] = 0x0000; // Controlword: initial state
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

                            // Handle CIA402 controlword (0x6040) - update state machine
                            if (index == 0x6040) {
                                process_controlword(static_cast<uint16_t>(value));
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

            // CIA402 state machine implementation
            void process_controlword(uint16_t controlword) {
                using namespace canopen::cia402;

                // Extract command bits from controlword
                uint8_t cmd_bits = controlword & 0x8F;

                switch (current_state_) {
                case State::SWITCH_ON_DISABLED:
                    if ((cmd_bits & 0x0F) == 0x06) {     // Shutdown command
                        current_state_ = State::READY_TO_SWITCH_ON;
                    }
                    break;

                case State::READY_TO_SWITCH_ON:
                    if ((cmd_bits & 0x0F) == 0x07) {     // Switch On command
                        current_state_ = State::SWITCHED_ON;
                    } else if ((cmd_bits & 0x0F) == 0x00) {     // Disable Voltage
                        current_state_ = State::SWITCH_ON_DISABLED;
                    }
                    break;

                case State::SWITCHED_ON:
                    if ((cmd_bits & 0x0F) == 0x0F) {     // Enable Operation command
                        current_state_ = State::OPERATION_ENABLED;
                    } else if ((cmd_bits & 0x0F) == 0x06) {     // Shutdown
                        current_state_ = State::READY_TO_SWITCH_ON;
                    } else if ((cmd_bits & 0x0F) == 0x00) {     // Disable Voltage
                        current_state_ = State::SWITCH_ON_DISABLED;
                    }
                    break;

                case State::OPERATION_ENABLED:
                    if ((cmd_bits & 0x0F) == 0x07) {     // Disable Operation command
                        current_state_ = State::SWITCHED_ON;
                    } else if ((cmd_bits & 0x0F) == 0x06) {     // Shutdown
                        current_state_ = State::READY_TO_SWITCH_ON;
                    } else if ((cmd_bits & 0x0F) == 0x00) {     // Disable Voltage
                        current_state_ = State::SWITCH_ON_DISABLED;
                    }
                    break;

                default:
                    break;
                }

                // Update statusword to reflect new state
                update_statusword_from_state();
            }

            void update_statusword_from_state() {
                using namespace canopen::cia402;

                uint16_t statusword = 0;

                switch (current_state_) {
                case State::NOT_READY_TO_SWITCH_ON:
                    statusword = 0x0000;
                    break;
                case State::SWITCH_ON_DISABLED:
                    statusword = 0x0040;
                    break;
                case State::READY_TO_SWITCH_ON:
                    statusword = 0x0021;
                    break;
                case State::SWITCHED_ON:
                    statusword = 0x0023;
                    break;
                case State::OPERATION_ENABLED:
                    statusword = 0x0027;
                    break;
                case State::QUICK_STOP_ACTIVE:
                    statusword = 0x0007;
                    break;
                case State::FAULT_REACTION_ACTIVE:
                    statusword = 0x000F;
                    break;
                case State::FAULT:
                    statusword = 0x0008;
                    break;
                case State::UNKNOWN:
                    statusword = 0x0000;
                    break;
                }

                // Add target reached and voltage enabled bits
                statusword |= (1 << 10); // Target reached
                statusword |= (1 << 9);  // Remote (CAN control)

                registers_[0x6041] = statusword;
            }

            uint8_t node_id_;
            std::atomic<bool> running_;
            std::shared_ptr<waveshare::ICANSocket> socket_;
            std::thread responder_thread_;
            canopen::cia402::State current_state_;

            mutable std::mutex registers_mutex_;
            std::map<uint16_t, uint32_t> registers_;
    };

/**
 * @brief RAII fixture for CANopen integration tests with automatic mock motor
 *
 * Usage with Catch2:
 *
 * TEST_CASE_METHOD(CANopenIntegrationFixture, "My test", "[integration]") {
 *     // mock_motor is already running on vcan_test with node ID 127
 *     auto socket = create_test_socket("vcan_test");
 *     auto dict = load_test_dictionary();
 *     SDOClient client(socket, dict, 127);  // Use node ID 127 for mock motor
 *     // ... test code ...
 * }
 */
    class CANopenIntegrationFixture {
        protected:
            CANopenIntegrationFixture(uint8_t node_id = 127)
                : mock_motor(node_id, "vcan_test") {
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
                    auto test_socket = create_test_socket("vcan_test");
                    return test_socket && test_socket->is_open();
                } catch (...) {
                    return false;
                }
            }

            MockMotorResponder mock_motor;
    };

} // namespace test_utils

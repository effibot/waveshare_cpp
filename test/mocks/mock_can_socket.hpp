/**
 * @file mock_can_socket.hpp
 * @brief Mock implementation of ICANSocket for testing
 * @version 1.0
 * @date 2025-10-14
 *
 * Provides queue-based simulation of SocketCAN I/O for testing SocketCANBridge
 * and related components without hardware.
 */

#pragma once

#include "../../include/io/can_socket.hpp"
#include "../../include/enums/error.hpp"
#include "../../include/exception/waveshare_exception.hpp"
#include <linux/can.h>
#include <queue>
#include <vector>
#include <cerrno>
#include <cstring>
#include <cstdint>

namespace waveshare {
    namespace test {

        /**
         * @brief Mock CAN socket for testing
         *
         * Features:
         * - Queue-based RX/TX simulation for can_frame
         * - TX history tracking for verification
         * - Configurable error injection (timeout, I/O errors)
         * - No actual SocketCAN hardware required
         */
        class MockCANSocket : public ICANSocket {
            public:
                /**
                 * @brief Construct mock CAN socket
                 * @param interface_name Simulated interface name (e.g., "vcan0", "mock0")
                 * @param timeout_ms Simulated timeout (not actually used in mock)
                 */
                MockCANSocket(const std::string& interface_name, int timeout_ms)
                    : interface_name_(interface_name)
                    , timeout_ms_(timeout_ms)
                    , is_open_(true)
                    , fd_(100)  // Mock FD (different from serial to avoid conflicts)
                    , simulate_timeout_(false)
                    , simulate_send_error_(false)
                    , simulate_receive_error_(false) {}

                ~MockCANSocket() override {
                    close();
                }

                // === ICANSocket Interface ===

                ssize_t send(const can_frame& frame) override {
                    if (!is_open_) {
                        errno = EBADF;
                        return -1;
                    }

                    if (simulate_send_error_) {
                        errno = EIO;
                        return -1;
                    }

                    // Record transmitted frame
                    tx_history_.push_back(frame);

                    return sizeof(can_frame);
                }

                ssize_t receive(can_frame& frame) override {
                    if (!is_open_) {
                        errno = EBADF;
                        return -1;
                    }

                    if (simulate_receive_error_) {
                        errno = EIO;
                        return -1;
                    }

                    if (simulate_timeout_ || rx_queue_.empty()) {
                        errno = EAGAIN;  // Timeout
                        return -1;
                    }

                    // Get next frame from queue
                    frame = rx_queue_.front();
                    rx_queue_.pop();

                    return sizeof(can_frame);
                }

                bool is_open() const override {
                    return is_open_;
                }

                void close() override {
                    is_open_ = false;
                    fd_ = -1;
                }

                std::string get_interface_name() const override {
                    return interface_name_;
                }

                int get_fd() const override {
                    return fd_;
                }

                // === Mock Control Interface ===

                /**
                 * @brief Inject CAN frame into RX queue (simulates receiving frame)
                 * @param frame CAN frame to inject
                 */
                void inject_rx_frame(const can_frame& frame) {
                    rx_queue_.push(frame);
                }

                /**
                 * @brief Inject multiple CAN frames into RX queue
                 * @param frames Vector of frames to inject
                 */
                void inject_rx_frames(const std::vector<can_frame>& frames) {
                    for (const auto& frame : frames) {
                        rx_queue_.push(frame);
                    }
                }

                /**
                 * @brief Get history of all transmitted CAN frames
                 * @return Vector of transmitted frames
                 */
                const std::vector<can_frame>& get_tx_history() const {
                    return tx_history_;
                }

                /**
                 * @brief Clear TX history
                 */
                void clear_tx_history() {
                    tx_history_.clear();
                }

                /**
                 * @brief Clear RX queue
                 */
                void clear_rx_queue() {
                    while (!rx_queue_.empty()) {
                        rx_queue_.pop();
                    }
                }

                /**
                 * @brief Enable/disable timeout simulation
                 * @param enable If true, receive() will return -1 with errno=EAGAIN
                 */
                void set_simulate_timeout(bool enable) {
                    simulate_timeout_ = enable;
                }

                /**
                 * @brief Enable/disable send error simulation
                 * @param enable If true, send() will return -1 with errno=EIO
                 */
                void set_simulate_send_error(bool enable) {
                    simulate_send_error_ = enable;
                }

                /**
                 * @brief Enable/disable receive error simulation
                 * @param enable If true, receive() will return -1 with errno=EIO
                 */
                void set_simulate_receive_error(bool enable) {
                    simulate_receive_error_ = enable;
                }

                /**
                 * @brief Get number of frames waiting in RX queue
                 * @return Size of RX queue
                 */
                std::size_t get_rx_queue_size() const {
                    return rx_queue_.size();
                }

                /**
                 * @brief Create a CAN frame with given ID and data
                 * @param can_id CAN ID
                 * @param data Data bytes (max 8)
                 * @return Constructed can_frame
                 */
                static can_frame make_frame(uint32_t can_id, const std::vector<uint8_t>& data) {
                    can_frame frame{};
                    frame.can_id = can_id;
                    frame.can_dlc = std::min(static_cast<uint8_t>(data.size()),
                        static_cast<uint8_t>(8));
                    std::memcpy(frame.data, data.data(), frame.can_dlc);
                    return frame;
                }

            private:
                std::string interface_name_;
                int timeout_ms_;
                bool is_open_;
                int fd_;

                // RX simulation
                std::queue<can_frame> rx_queue_;

                // TX tracking
                std::vector<can_frame> tx_history_;

                // Error injection
                bool simulate_timeout_;
                bool simulate_send_error_;
                bool simulate_receive_error_;
        };

    } // namespace test
} // namespace waveshare

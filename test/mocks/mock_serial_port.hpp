/**
 * @file mock_serial_port.hpp
 * @brief Mock implementation of ISerialPort for testing
 * @version 1.0
 * @date 2025-10-14
 *
 * Provides queue-based simulation of serial port I/O for testing USBAdapter
 * and related components without hardware.
 */

#pragma once

#include "../../include/io/serial_port.hpp"
#include "../../include/enums/error.hpp"
#include "../../include/exception/waveshare_exception.hpp"
#include <queue>
#include <vector>
#include <cstring>
#include <cerrno>
#include <cstdint>

namespace waveshare {
    namespace test {

        /**
         * @brief Mock serial port for testing
         *
         * Features:
         * - Queue-based RX/TX simulation
         * - TX history tracking for verification
         * - Configurable error injection (timeout, I/O errors)
         * - No actual hardware required
         */
        class MockSerialPort : public ISerialPort {
            public:
                /**
                 * @brief Construct mock serial port
                 * @param device_path Simulated device path (e.g., "/dev/mock")
                 */
                explicit MockSerialPort(const std::string& device_path)
                    : device_path_(device_path)
                    , is_open_(true)
                    , fd_(42)  // Mock FD
                    , simulate_timeout_(false)
                    , simulate_write_error_(false)
                    , simulate_read_error_(false) {}

                ~MockSerialPort() override {
                    close();
                }

                // === ISerialPort Interface ===

                ssize_t write(const void* data, std::size_t len) override {
                    if (!is_open_) {
                        errno = EBADF;
                        return -1;
                    }

                    if (simulate_write_error_) {
                        errno = EIO;
                        return -1;
                    }

                    // Record transmitted data
                    const uint8_t* bytes = static_cast<const uint8_t*>(data);
                    std::vector<uint8_t> frame(bytes, bytes + len);
                    tx_history_.push_back(frame);

                    return static_cast<ssize_t>(len);
                }

                ssize_t read(void* data, std::size_t len, int timeout_ms) override {
                    (void)timeout_ms;  // Mock doesn't use timeout for simplicity

                    if (!is_open_) {
                        errno = EBADF;
                        return -1;
                    }

                    if (simulate_read_error_) {
                        errno = EIO;
                        return -1;
                    }

                    if (simulate_timeout_ || rx_queue_.empty()) {
                        errno = EAGAIN;  // Timeout
                        return -1;
                    }

                    // Get next frame from queue
                    auto frame = rx_queue_.front();
                    rx_queue_.pop();

                    std::size_t bytes_to_copy = std::min(len, frame.size());
                    std::memcpy(data, frame.data(), bytes_to_copy);

                    return static_cast<ssize_t>(bytes_to_copy);
                }

                bool is_open() const override {
                    return is_open_;
                }

                void close() override {
                    is_open_ = false;
                    fd_ = -1;
                }

                std::string get_device_path() const override {
                    return device_path_;
                }

                int get_fd() const override {
                    return fd_;
                }

                // === Mock Control Interface ===

                /**
                 * @brief Inject data into RX queue (simulates receiving data)
                 * @param data Data to inject
                 */
                void inject_rx_data(const std::vector<uint8_t>& data) {
                    rx_queue_.push(data);
                }

                /**
                 * @brief Inject multiple frames into RX queue
                 * @param frames Vector of frames to inject
                 */
                void inject_rx_frames(const std::vector<std::vector<uint8_t> >& frames) {
                    for (const auto& frame : frames) {
                        rx_queue_.push(frame);
                    }
                }

                /**
                 * @brief Get history of all transmitted data
                 * @return Vector of transmitted frames
                 */
                const std::vector<std::vector<uint8_t> >& get_tx_history() const {
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
                 * @param enable If true, read() will return EAGAIN
                 */
                void set_simulate_timeout(bool enable) {
                    simulate_timeout_ = enable;
                }

                /**
                 * @brief Enable/disable write error simulation
                 * @param enable If true, write() will return -1 with errno=EIO
                 */
                void set_simulate_write_error(bool enable) {
                    simulate_write_error_ = enable;
                }

                /**
                 * @brief Enable/disable read error simulation
                 * @param enable If true, read() will return -1 with errno=EIO
                 */
                void set_simulate_read_error(bool enable) {
                    simulate_read_error_ = enable;
                }

                /**
                 * @brief Get number of frames waiting in RX queue
                 * @return Size of RX queue
                 */
                std::size_t get_rx_queue_size() const {
                    return rx_queue_.size();
                }

            private:
                std::string device_path_;
                bool is_open_;
                int fd_;

                // RX simulation
                std::queue<std::vector<uint8_t> > rx_queue_;

                // TX tracking
                std::vector<std::vector<uint8_t> > tx_history_;

                // Error injection
                bool simulate_timeout_;
                bool simulate_write_error_;
                bool simulate_read_error_;
        };

    } // namespace test
} // namespace waveshare

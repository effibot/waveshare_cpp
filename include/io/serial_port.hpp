/**
 * @file serial_port.hpp
 * @brief Abstract interface for serial port I/O operations
 * @version 1.0
 * @date 2025-10-14
 *
 * Provides abstraction for serial port operations to enable dependency injection
 * and mock-based testing. This interface focuses on low-level I/O operations,
 * while higher-level protocol logic remains in USBAdapter.
 */

#pragma once

#include <cstddef>
#include <string>

namespace waveshare {

    /**
     * @brief Abstract interface for serial port I/O
     *
     * This interface abstracts low-level serial port operations (read/write bytes)
     * to allow for dependency injection and testing with mocks.
     *
     * Implementations:
     * - RealSerialPort: Uses termios/ioctl for actual hardware
     * - MockSerialPort: Queue-based simulation for testing
     */
    class ISerialPort {
        public:
            virtual ~ISerialPort() = default;

            /**
             * @brief Write data to serial port
             * @param data Pointer to data buffer
             * @param len Number of bytes to write
             * @return ssize_t Bytes written, or -1 on error (sets errno)
             */
            virtual ssize_t write(const void* data, std::size_t len) = 0;

            /**
             * @brief Read data from serial port with timeout
             * @param data Pointer to buffer for received data
             * @param len Maximum number of bytes to read
             * @param timeout_ms Timeout in milliseconds (-1 for blocking)
             * @return ssize_t Bytes read, or -1 on error/timeout (sets errno)
             *
             * On timeout, returns -1 with errno set to EAGAIN or EWOULDBLOCK.
             */
            virtual ssize_t read(void* data, std::size_t len, int timeout_ms) = 0;

            /**
             * @brief Check if serial port is open and ready
             * @return bool True if port is open
             */
            virtual bool is_open() const = 0;

            /**
             * @brief Close the serial port
             */
            virtual void close() = 0;

            /**
             * @brief Get the device path
             * @return std::string Device path (e.g., "/dev/ttyUSB0")
             */
            virtual std::string get_device_path() const = 0;

            /**
             * @brief Get the file descriptor (for select/poll operations)
             * @return int File descriptor, or -1 if not open
             */
            virtual int get_fd() const = 0;
    };

} // namespace waveshare

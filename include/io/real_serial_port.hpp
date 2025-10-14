/**
 * @file real_serial_port.hpp
 * @brief Real serial port implementation using termios/ioctl
 * @version 1.0
 * @date 2025-10-14
 */

#pragma once

#include "serial_port.hpp"
#include "../enums/protocol.hpp"
#include "../exception/waveshare_exception.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <asm/termbits.h>
#include <cstring>
#include <cerrno>

namespace waveshare {

    /**
     * @brief Real serial port implementation using Linux termios
     *
     * Wraps low-level termios/ioctl operations for actual hardware communication.
     * Extracted from USBAdapter to enable dependency injection and testing.
     */
    class RealSerialPort : public ISerialPort {
        private:
            std::string device_path_;
            SerialBaud baud_rate_;
            int fd_ = -1;
            struct termios2 tty_ {};
            bool is_open_ = false;

        public:
            /**
             * @brief Construct and open serial port
             * @param device_path Device path (e.g., "/dev/ttyUSB0")
             * @param baud_rate Serial baud rate
             * @throws DeviceException if port cannot be opened or configured
             */
            RealSerialPort(const std::string& device_path, SerialBaud baud_rate);

            /**
             * @brief Destructor - closes port if open
             */
            ~RealSerialPort() override;

            // Disable copy
            RealSerialPort(const RealSerialPort&) = delete;
            RealSerialPort& operator=(const RealSerialPort&) = delete;

            // ISerialPort implementation
            ssize_t write(const void* data, std::size_t len) override;
            ssize_t read(void* data, std::size_t len, int timeout_ms) override;
            bool is_open() const override { return is_open_; }
            void close() override;
            std::string get_device_path() const override { return device_path_; }
            int get_fd() const override { return fd_; }

        private:
            /**
             * @brief Open the serial port
             * @throws DeviceException on failure
             */
            void open_port();

            /**
             * @brief Configure serial port settings (baud rate, etc.)
             * @throws DeviceException on failure
             */
            void configure_port();
    };

} // namespace waveshare

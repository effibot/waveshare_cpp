/**
 * @file usb_adapter.hpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief Provides adapter interface to interact with the Waveshare USB adapter
 * @version 0.1
 * @date 2025-10-10
 *
 * @copyright Copyright (c) 2025
 *
 */


#pragma once

#include "../enums/protocol.hpp"
#include "../template/frame_traits.hpp"
#include "../frame/config_frame.hpp"
#include "../frame/fixed_frame.hpp"
#include "../frame/variable_frame.hpp"
#include <stdexcept>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <asm/termbits.h>
#include <cstring>
#include <csignal>
#include <iomanip>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include "../io/serial_port.hpp"
#include <memory>

namespace waveshare {

    /**
     * @brief USB-CAN Adapter interface
     *
     * This class provides methods to interact with the Waveshare USB-CAN adapter.
     * It abstracts low-level USB communication and provides high-level methods
     * for sending and receiving CAN frames.
     *
     * @note This implementation uses dependency injection for serial I/O to enable
     * testing with mocks. Production code uses RealSerialPort for hardware communication.
     */
    class USBAdapter {

        private:
            // # I/O abstraction
            std::unique_ptr<ISerialPort> serial_port_;  // Injected serial port (real or mock)

            // # Internal State
            std::string usb_device_;    // e.g., "/dev/ttyUSB0"
            SerialBaud baudrate_;   // e.g., SerialBaud::BAUD_2M
            bool is_configured_ = false; // Flag to indicate if the port is configured
            bool is_monitoring_ = false;  // Flag to indicate if monitoring is active
            static inline volatile std::sig_atomic_t stop_flag = false; // Flag to indicate if a stop signal was received

            // # Thread-safety primitives
            mutable std::shared_mutex state_mutex_;  // Protects is_configured_
            std::mutex write_mutex_;                 // Exclusive write lock
            std::mutex read_mutex_;                  // Exclusive read lock

            // # Internal utility methods

            /**
             * @brief Signal handler for SIGINT
             *
             * This function is called when the program receives a SIGINT signal.
             * It sets the stop_flag to true, which can be used to gracefully
             * terminate the program.
             *
             * @param signum The signal number (unused)
             */
            static void sigint_handler(int signum);

            // # Internal utility methods (removed old port management - now in RealSerialPort)

            // === Low-level I/O operations (thread-safe) ===

            /**
             * @brief Write raw bytes to the serial port (thread-safe)
             *
             * This function writes data to the serial port with exclusive write access.
             * Multiple threads can safely call this method concurrently.
             *
             * @param data Pointer to data buffer
             * @param size Number of bytes to write
             * @return int Number of bytes written
             * @throws DeviceException if port not open or write fails
             * @throws ProtocolException if size is invalid (0 or > MAX_BUFFER)
             */
            int write_bytes(const std::uint8_t* data, std::size_t size);

            /**
             * @brief Read raw bytes from the serial port (thread-safe)
             *
             * This function reads data from the serial port with exclusive read access.
             * Multiple threads can safely call this method concurrently.
             * Uses non-blocking read with termios timeout (VTIME).
             *
             * @param buffer Pointer to buffer to store read data
             * @param size Maximum number of bytes to read
             * @return int Number of bytes actually read (may be 0 if no data available)
             * @throws DeviceException if port not open or read fails
             * @throws ProtocolException if size is invalid (0 or > MAX_BUFFER)
             */
            int read_bytes(std::uint8_t* buffer, std::size_t size);

            /**
             * @brief Read exact number of bytes with timeout (thread-safe)
             *
             * Repeatedly calls read_bytes() until exactly 'size' bytes are read or timeout expires.
             * Useful for reading fixed-size frames (e.g., 20-byte FixedFrame/ConfigFrame).
             *
             * @param buffer Destination buffer (must have capacity >= size)
             * @param size Exact number of bytes required
             * @param timeout_ms Total timeout in milliseconds
             * @throws TimeoutException if timeout expires before all bytes read
             * @throws DeviceException if port not open or read fails
             * @throws ProtocolException if size is invalid
             */
            void read_exact(std::uint8_t* buffer, std::size_t size, int timeout_ms);


        public:
            /**
             * @brief Constructor for USBAdapter with dependency injection
             * @param serial_port Injected serial port (real or mock)
             * @param usb_dev Device path for identification
             * @param baudrate Serial baud rate (for logging/reference)
             */
            USBAdapter(std::unique_ptr<ISerialPort> serial_port, std::string usb_dev,
                SerialBaud baudrate = DEFAULT_SERIAL_BAUD);

            /**
             * @brief Factory method to create USBAdapter with real hardware
             * @param usb_dev Device path (e.g., "/dev/ttyUSB0")
             * @param baudrate Serial baud rate
             * @return std::unique_ptr<USBAdapter> Configured adapter ready to use
             */
            static std::unique_ptr<USBAdapter> create(const std::string& usb_dev,
                SerialBaud baudrate = DEFAULT_SERIAL_BAUD);

            // USBAdapter instance



            /**
             * @brief Destructor
             *
             * Serial port is automatically closed by ISerialPort destructor
             */
            ~USBAdapter() = default;

            // # Set/Get methods

            /**
             * @brief Get the configured baud rate
             *
             * @return SerialBaud The baud rate set during construction
             */
            SerialBaud get_baudrate() const {
                return baudrate_;
            }

            /**
             * @brief Set the baudrate object
             *
             * @param baudrate
             */
            void set_baudrate(SerialBaud baudrate) { baudrate_ = baudrate; }
            /**
             * @brief Get the name of USB device object
             * @return std::string
             */

            std::string get_usb_device() const { return usb_device_; }

            /**
             * @brief Set the name of USB device object
             * @param usb_device
             * @note This will close the current port if open. Call create() to reopen with new device.
             */
            void set_usb_device(const std::string& usb_device) {
                // Close current port
                if (serial_port_) {
                    serial_port_->close();
                    serial_port_.reset();
                }
                // Set the port as not configured
                is_configured_ = false;
                // Set the new device name
                usb_device_ = usb_device;
            }

            /**
             * @brief Check if the USB device is open
             *
             * @return true if the device is open, false otherwise
             */
            bool is_open() const {
                return serial_port_ && serial_port_->is_open();
            }

            /**
             * @brief Get the file descriptor of the serial port
             *
             * @return int The file descriptor, or -1 if not open
             */
            int get_fd() const {
                return serial_port_ ? serial_port_->get_fd() : -1;
            }

            /**
             * @brief Check if the port is configured
             *
             * @return true if the port is configured, false otherwise
             */
            bool is_configured() const { return is_configured_; }

            /**
             * @brief Check if a stop signal has been received
             *
             * This function checks if the stop_flag has been set to true,
             * indicating that a SIGINT signal has been received.
             *
             * @return true if a stop signal has been received, false otherwise
             */
            static bool should_stop() { return stop_flag; }

            std::string to_string() const {
                std::shared_lock<std::shared_mutex> lock(state_mutex_);
                std::ostringstream oss;
                oss << "USBAdapter(";
                oss << "Device: " << usb_device_ << ", ";
                oss << "Baudrate: " << static_cast<int>(baudrate_) << ", ";
                oss << "FD: " << (serial_port_ ? serial_port_->get_fd() : -1) << ", ";
                oss << "Open: " << (serial_port_ && serial_port_->is_open() ? "Yes" : "No") << ", ";
                oss << "Configured: " << (is_configured_ ? "Yes" : "No");
                oss << ")";
                return oss.str();
            }

            // === Frame-Level API ===

            /**
             * @brief Send a Waveshare Frame to the USB adapter
             *
             * Serialize the given frame and writes it to serial port atomically.
             * @note This method is thread-safe and multiple threads can call it concurrently.
             * @tparam Frame the frame object from which to serialize data
             * @param frame the frame object to send
             * @throws DeviceException if port not open or write fails
             * @throws ProtocolException if partial write occurs
             */
            template<typename Frame>
            int send_frame(const Frame& frame) {
                // State-First: Generate protocol buffer from frame state
                auto buffer = frame.serialize();

                // Write all bytes atomically (thread-safe via write_mutex_)
                int bytes_written = write_bytes(buffer.data(), buffer.size());

                // Verify all bytes written
                if (bytes_written != static_cast<int>(buffer.size())) {
                    throw ProtocolException(Status::DNOT_OPEN,
                        "send_frame: Partial write " + std::to_string(bytes_written) +
                        "/" + std::to_string(buffer.size()));
                }
                return bytes_written;
            }

            /**
             * @brief Receive a fixed-size Waveshare data frame from the USB adapter
             * This method reads exactly 20 bytes from the serial port and parses it into a FixedFrame object using deserialize().
             * @note This method is thread-safe and multiple threads can call it concurrently.
             *
             * @param timeout_ms maximum time to wait for the full frame (in milliseconds). Defaults to 1000ms.
             * @return FixedFrame The received and parsed frame
             * @throws TimeoutException if timeout expires before frame received
             * @throws DeviceException if port not open or read fails
             * @throws ProtocolException if frame deserialization fails
             */
            FixedFrame receive_fixed_frame(int timeout_ms = 1000);

            /**
             * @brief Receive a variable-size data frame from the USB adapter
             * This method reads byte-by-byte until complete VariableFrame detected:
             *
             * -START byte (0xAA) detected
             *
             * -Reads until END byte (0x55) found
             *
             * -Deserializes complete frame
             *
             * @note This method is thread-safe and multiple threads can call it concurrently.
             *
             * @param timeout_ms maximum time to wait for the full frame (in milliseconds). Defaults to 1000ms.
             * @return VariableFrame The received and parsed frame
             * @throws TimeoutException if timeout expires before frame received
             * @throws DeviceException if port not open or read fails
             * @throws ProtocolException if frame deserialization fails
             */

            VariableFrame receive_variable_frame(int timeout_ms = 1000);
    };

}     // namespace USBCANBridge
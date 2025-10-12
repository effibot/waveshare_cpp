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
#include "../template/result.hpp"
#include "../template/frame_traits.hpp"
#include <stdexcept>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <csignal>
#include <iomanip>
#include <mutex>
#include <shared_mutex>
#include <chrono>

namespace USBCANBridge {

    /**
     * @brief USB-CAN Adapter interface
     *
     * This class provides methods to interact with the Waveshare USB-CAN adapter.
     * It abstracts low-level USB communication and provides high-level methods
     * for sending and receiving CAN frames.
     *
     * @note This implementation relies on the fact that the Linux kernel exposes those adapter
     * under /dev/ttyUSBx as a serial device, loading the `ch341-uart` driver.
     * Under Windows, the driver is provided in the official Waveshare website.
     * See the project README for details.
     */
    class USBAdapter {

        private:
            // # Internal State
            std::string usb_device_;    // e.g., "/dev/ttyUSB0"
            SerialBaud baudrate_;   // e.g., SerialBaud::BAUD_2M
            int fd_ = -1;  // File descriptor for the serial port
            struct termios tty_ {}; // Terminal control structure
            bool is_open_ = false;  // Flag to indicate if the port is open
            bool is_configured_ = false; // Flag to indicate if the port is configured
            bool is_monitoring_ = false;  // Flag to indicate if monitoring is active
            static inline volatile std::sig_atomic_t stop_flag = false; // Flag to indicate if a stop signal was received

            // # Thread-safety primitives
            mutable std::shared_mutex state_mutex_;  // Protects is_open_, is_configured_, fd_
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

            /**
             * @brief Configure the serial port settings
             *
             * This function configures the serial port settings such as baud rate,
             * character size, and parity. It uses the termios structure to set
             * the desired parameters.
             * Those parameters are set according to the adapter C example code.
             * @throws std::runtime_error if the configuration fails
             * @note This function should be called after opening the port.
             */
            Result<void> configure_port();

            /**
             * @brief Close the serial port
             *
             * This function closes the serial port if it is open. It also resets
             * the is_open_ flag to false.
             * @throws std::runtime_error if closing the port fails
             * @note This function should be called in the destructor to ensure
             * proper resource cleanup.
             */
            Result<void> close_port();

            /**
             * @brief Open the serial port
             *
             * This function opens the serial port specified by usb_device_.
             * It sets the file descriptor and configures the port settings.
             * @throws std::runtime_error if opening or configuring the port fails
             * @note This function should be called before any read/write operations.
             */
            Result<void> open_port();

            // === Low-level I/O operations (thread-safe) ===

            /**
             * @brief Write raw bytes to the serial port (thread-safe)
             *
             * This function writes data to the serial port with exclusive write access.
             * Multiple threads can safely call this method concurrently.
             *
             * @param data Pointer to data buffer
             * @param size Number of bytes to write
             * @return Result<int> Number of bytes written on success, or error status
             */
            Result<int> write_bytes(const std::uint8_t* data, std::size_t size);

            /**
             * @brief Read raw bytes from the serial port (thread-safe)
             *
             * This function reads data from the serial port with exclusive read access.
             * Multiple threads can safely call this method concurrently.
             * Uses non-blocking read with termios timeout (VTIME).
             *
             * @param buffer Pointer to buffer to store read data
             * @param size Maximum number of bytes to read
             * @return Result<int> Number of bytes actually read on success (may be 0 if no data available), or error status
             */
            Result<int> read_bytes(std::uint8_t* buffer, std::size_t size);

            /**
             * @brief Read exact number of bytes with timeout (thread-safe)
             *
             * Repeatedly calls read_bytes() until exactly 'size' bytes are read or timeout expires.
             * Useful for reading fixed-size frames (e.g., 20-byte FixedFrame/ConfigFrame).
             *
             * @param buffer Destination buffer (must have capacity >= size)
             * @param size Exact number of bytes required
             * @param timeout_ms Total timeout in milliseconds
             * @return Result<void> Success if all bytes read, Status::DTIMEOUT on timeout, or read error
             */
            Result<void> read_exact(std::uint8_t* buffer, std::size_t size, int timeout_ms);

            /**
             * @brief Flush the serial port buffers (thread-safe)
             *
             * This function flushes both input and output buffers of the serial port.
             * Acquires both read and write locks to ensure no concurrent I/O during flush.
             *
             * @return Result<void> Success or error status
             */
            Result<void> flush_port();


        public:
            /**
             * @brief Constructor for USBAdapter
             */
            USBAdapter(std::string usb_dev, SerialBaud baudrate = DEFAULT_SERIAL_BAUD) :
                usb_device_(std::move(usb_dev)), baudrate_(baudrate) {

                // Register the SIGINT handler
                std::signal(SIGINT, sigint_handler);

                // Open and configure the port
                auto res = open_port();
                if (res.fail()) {
                    throw std::runtime_error("Failed to open port: " + res.describe());
                }

                res = configure_port();
                if (res.fail()) {
                    throw std::runtime_error("Failed to configure port: " + res.describe());
                }

                // Flush any existing data
                flush_port();
            }


            /**
             * @brief Destructor for USBAdapter
             *
             * This destructor ensures that the serial port is closed properly
             * when the USBAdapter object goes out of scope.
             */
            ~USBAdapter() {
                if (is_open_) {
                    close_port();
                }
            }

            // # Set/Get methods

            /**
             * @brief Get the baudrate object
             *
             * @return SerialBaud
             */
            SerialBaud get_baudrate() const { return baudrate_; }
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
             */
            void set_usb_device(const std::string& usb_device) {
                // If the port is open, close it first
                if (is_open_) {
                    close_port();
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
            bool is_open() const { return is_open_; }

            /**
             * @brief Get the file descriptor of the serial port
             *
             * @return int The file descriptor, or -1 if not open
             */
            int get_fd() const { return fd_; }

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


            // === Frame-Level API ===

            /**
             * @brief Send a Waveshare Frame to the USB adapter
             *
             * Serialize the given frame and writes it to serial port atomically.
             * @note This method is thread-safe and multiple threads can call it concurrently.
             * @tparam Frame the frame object from which to serialize data
             * @param frame the frame object to send
             * @return Result<void> success or error status
             */
            template<typename Frame>
            Result<void> send_frame(const Frame& frame);

            /**
             * @brief Receive a fixed-size Waveshare data frame from the USB adapter
             * This method reads exactly 20 bytes from the serial port and parses it into a FixedFrame object using deserialize().
             * @note This method is thread-safe and multiple threads can call it concurrently.
             *
             * @tparam Frame which must be FixedFrame
             * @param timeout_ms maximum time to wait for the full frame (in milliseconds). Defaults to 1000ms.
             * @return Result<FixedFrame> success or error status
             */
            template<typename Frame>
            Result<Frame> receive_fixed_frame(int timeout_ms = 1000);

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
             * @tparam Frame which must be VariableFrame
             * @param timeout_ms maximum time to wait for the full frame (in milliseconds). Defaults to 1000ms.
             * @return Result<VariableFrame> success or error status
             */
            template<typename Frame>
            Result<Frame> receive_variable_frame(int timeout_ms = 1000);
    };
}     // namespace USBCANBridge
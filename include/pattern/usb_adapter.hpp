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
#include <stdexcept>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <csignal>
#include <iomanip>

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
            std::string usb_device_;
            SerialBaud baudrate_;
            int fd_ = -1;  // File descriptor for the serial port
            struct termios tty_ {};
            bool is_open_ = false;
            volatile std::sig_atomic_t stop_flag = false;

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
            void sigint_handler(int);

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

            // Low-level read/write operations
            void write_bytes(const std::uint8_t* data, std::size_t size);
            void read_bytes(std::uint8_t* buffer, std::size_t size);
            void flush_port();


        public:
            /**
             * @brief Constructor for USBAdapter
             */
            USBAdapter(std::string usb_dev, SerialBaud baudrate = DEFAULT_SERIAL_BAUD) :
                usb_device_(std::move(usb_dev)), baudrate_(baudrate) {

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

            SerialBaud get_baudrate() const { return baudrate_; }
            std::string get_usb_device() const { return usb_device_; }
            bool is_open() const { return is_open_; }
            int get_fd() const { return fd_; }




    };
}     // namespace USBCANBridge
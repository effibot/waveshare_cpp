/**
 * @file usb_adapter.cpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief USB-CAN Adapter implementation
 * @version 0.1
 * @date 2025-10-10
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "../include/pattern/usb_adapter.hpp"

namespace USBCANBridge {

    void USBAdapter::sigint_handler(int) { stop_flag = true; }

    Result<void> USBAdapter::configure_port() {

        // If the port is not open, return an error
        if (!is_open_ || fd_ == -1) {
            return Result<void>::error(Status::DNOT_OPEN, "configure_port");
        }

        // Clear the struct for new port settings
        memset(&tty_, 0, sizeof(tty_));

        // Get the current attributes of the Serial port
        if (tcgetattr(fd_, &tty_) != 0) {
            // Get errno and return as Status
            return Result<void>::error(Status::DNOT_OPEN, std::strerror(errno));
        }

        // Initialize the tty_ structure to raw mode
        cfmakeraw(&tty_);

        // Set I/O baud rate
        cfsetospeed(&tty_, to_speed_t(get_baudrate()));


        return Result<void>::success("Port configured successfully");
    }
}
/**
 * @file real_can_socket.hpp
 * @brief Real CAN socket implementation using Linux SocketCAN
 * @version 1.0
 * @date 2025-10-14
 */

#pragma once

#include "can_socket.hpp"
#include "../exception/waveshare_exception.hpp"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <cstring>

namespace waveshare {

    /**
     * @brief Real CAN socket implementation using Linux SocketCAN
     *
     * Wraps low-level SocketCAN socket operations for actual hardware communication.
     * Extracted from SocketCANBridge to enable dependency injection and testing.
     */
    class RealCANSocket : public ICANSocket {
        private:
            std::string interface_name_;
            int timeout_ms_;
            int fd_ = -1;
            bool is_open_ = false;

        public:
            /**
             * @brief Construct and open CAN socket
             * @param interface Interface name (e.g., "can0", "vcan0")
             * @param timeout_ms Receive timeout in milliseconds
             * @throws DeviceException if socket creation/binding fails
             */
            RealCANSocket(const std::string& interface, int timeout_ms);

            /**
             * @brief Destructor - closes socket if open
             */
            ~RealCANSocket() override;

            // Disable copy
            RealCANSocket(const RealCANSocket&) = delete;
            RealCANSocket& operator=(const RealCANSocket&) = delete;

            // ICANSocket implementation
            ssize_t send(const struct can_frame& frame) override;
            ssize_t receive(struct can_frame& frame) override;
            bool is_open() const override { return is_open_; }
            void close() override;
            std::string get_interface_name() const override { return interface_name_; }
            int get_fd() const override { return fd_; }

        private:
            /**
             * @brief Open and bind CAN socket
             * @throws DeviceException on failure
             */
            void open_socket();

            /**
             * @brief Set receive timeout on socket
             * @throws DeviceException on failure
             */
            void set_timeout();
    };

} // namespace waveshare

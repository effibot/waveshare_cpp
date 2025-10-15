/**
 * @file can_socket.hpp
 * @brief Abstract interface for CAN socket I/O operations
 * @version 1.0
 * @date 2025-10-14
 *
 * Provides abstraction for SocketCAN operations to enable dependency injection
 * and mock-based testing. This interface focuses on low-level socket I/O,
 * while higher-level frame conversion logic remains in SocketCANBridge.
 */

#pragma once

#include <linux/can.h>
#include <string>
#include <cstddef>

namespace waveshare {

    /**
     * @brief Abstract interface for CAN socket I/O
     *
     * This interface abstracts low-level SocketCAN socket operations (send/receive CAN frames)
     * to allow for dependency injection and testing with mocks.
     *
     * Implementations:
     * - RealCANSocket: Uses Linux SocketCAN for actual hardware
     * - MockCANSocket: Queue-based simulation for testing
     */
    class ICANSocket {
        public:
            virtual ~ICANSocket() = default;

            /**
             * @brief Send a CAN frame to the socket
             * @param frame CAN frame to send
             * @return ssize_t Bytes written, or -1 on error (sets errno)
             */
            virtual ssize_t send(const struct can_frame& frame) = 0;

            /**
             * @brief Receive a CAN frame from the socket
             * @param frame Reference to store received frame
             * @return ssize_t Bytes read, or -1 on error/timeout (sets errno)
             *
             * On timeout, returns -1 with errno set to EAGAIN or EWOULDBLOCK.
             */
            virtual ssize_t receive(struct can_frame& frame) = 0;

            /**
             * @brief Check if socket is open and ready
             * @return bool True if socket is open
             */
            virtual bool is_open() const = 0;

            /**
             * @brief Close the socket
             */
            virtual void close() = 0;

            /**
             * @brief Get the interface name
             * @return std::string Interface name (e.g., "vcan0", "can0")
             */
            virtual std::string get_interface_name() const = 0;

            /**
             * @brief Get the socket file descriptor (for select/poll if needed)
             * @return int Socket FD or -1 if not open
             */
            virtual int get_fd() const = 0;
    };

} // namespace waveshare

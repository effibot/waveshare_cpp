/**
 * @file real_can_socket.cpp
 * @brief Real CAN socket implementation
 * @version 1.0
 * @date 2025-10-14
 */

#include "../include/io/real_can_socket.hpp"
#include <cstdio>
#include <cerrno>

namespace waveshare {

    // ===================================================================
    // Constructor / Destructor
    // ===================================================================

    RealCANSocket::RealCANSocket(const std::string& interface, int timeout_ms)
        : interface_name_(interface), timeout_ms_(timeout_ms) {
        open_socket();
        set_timeout();
    }

    RealCANSocket::~RealCANSocket() {
        if (is_open_ && fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
            is_open_ = false;
        }
    }

    // ===================================================================
    // ICANSocket Implementation
    // ===================================================================

    ssize_t RealCANSocket::send(const struct can_frame& frame) {
        if (!is_open_ || fd_ < 0) {
            errno = ENOTCONN;
            return -1;
        }

        ssize_t bytes = ::write(fd_, &frame, sizeof(struct can_frame));
        return bytes;  // Returns -1 on error, errno set by write()
    }

    ssize_t RealCANSocket::receive(struct can_frame& frame) {
        if (!is_open_ || fd_ < 0) {
            errno = ENOTCONN;
            return -1;
        }

        ssize_t bytes = ::read(fd_, &frame, sizeof(struct can_frame));
        return bytes;  // Returns -1 on error/timeout, errno set by read()
    }

    void RealCANSocket::close() {
        if (is_open_ && fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
            is_open_ = false;
        }
    }

    // ===================================================================
    // Private Methods (Extracted from SocketCANBridge)
    // ===================================================================

    void RealCANSocket::open_socket() {
        // Create SocketCAN socket
        fd_ = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (fd_ < 0) {
            throw DeviceException(
                Status::DCONFIG_ERROR,
                "RealCANSocket::open_socket: Failed to create socket: " +
                std::string(std::strerror(errno))
            );
        }

        // Get interface index
        struct ifreq ifr;
        std::memset(&ifr, 0, sizeof(ifr));
        std::strncpy(ifr.ifr_name, interface_name_.c_str(), IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';  // Ensure null termination

        if (::ioctl(fd_, SIOCGIFINDEX, &ifr) < 0) {
            int saved_errno = errno;
            ::close(fd_);
            fd_ = -1;
            throw DeviceException(
                Status::DNOT_FOUND,
                "RealCANSocket::open_socket: Interface '" + interface_name_ + "' not found: " +
                std::string(std::strerror(saved_errno))
            );
        }

        // Bind socket to CAN interface
        struct sockaddr_can addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        if (::bind(fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
            int saved_errno = errno;
            ::close(fd_);
            fd_ = -1;
            throw DeviceException(
                Status::DCONFIG_ERROR,
                "RealCANSocket::open_socket: Failed to bind to '" + interface_name_ + "': " +
                std::string(std::strerror(saved_errno))
            );
        }

        is_open_ = true;
        std::fprintf(stdout, "CAN socket opened and bound to %s successfully.\n",
            interface_name_.c_str());
    }

    void RealCANSocket::set_timeout() {
        if (!is_open_ || fd_ < 0) {
            throw DeviceException(
                Status::DNOT_OPEN,
                "RealCANSocket::set_timeout: Socket not open"
            );
        }

        // Convert milliseconds to struct timeval
        struct timeval tv;
        tv.tv_sec = timeout_ms_ / 1000;
        tv.tv_usec = (timeout_ms_ % 1000) * 1000;

        // Set receive timeout
        if (::setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            throw DeviceException(
                Status::DCONFIG_ERROR,
                "RealCANSocket::set_timeout: Failed to set timeout: " +
                std::string(std::strerror(errno))
            );
        }

        std::fprintf(stdout, "CAN socket receive timeout set to %d ms.\n", timeout_ms_);
    }

} // namespace waveshare

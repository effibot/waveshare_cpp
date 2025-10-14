/**
 * @file socketcan_bridge.cpp
 * @brief SocketCAN bridge implementation
 * @version 1.0
 * @date 2025-10-14
 */

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>

#include "../include/pattern/socketcan_bridge.hpp"
#include "../include/interface/socketcan_helpers.hpp"
#include "../include/exception/waveshare_exception.hpp"
#include "../include/pattern/frame_builder.hpp"

namespace waveshare {

    // === Constructor & Destructor ===

    SocketCANBridge::SocketCANBridge(const BridgeConfig& config)
        : config_(config)
        , socketcan_fd_(-1)
        , adapter_(nullptr) {

        // Validate configuration
        config_.validate();

        // Open and configure SocketCAN socket
        socketcan_fd_ = open_socketcan_socket(config_.socketcan_interface);
        set_socketcan_timeouts();

        // Initialize and configure USB adapter (always required)
        initialize_usb_adapter();
        configure_usb_adapter();
    }

    SocketCANBridge::~SocketCANBridge() {
        // Stop threads if running
        try {
            if (running_.load(std::memory_order_relaxed)) {
                stop();
            }
        } catch (const std::exception& e) {
            std::cerr << "Error stopping bridge in destructor: "
                      << e.what() << std::endl;
        }

        // Gracefully close SocketCAN socket
        try {
            if (socketcan_fd_ >= 0) {
                close_socketcan_socket();
            }
        } catch (const std::exception& e) {
            // Log error but don't throw from destructor
            std::cerr << "Error closing SocketCAN socket in destructor: "
                      << e.what() << std::endl;
        }
    }

    // === Move Operations ===

    SocketCANBridge::SocketCANBridge(SocketCANBridge&& other) noexcept
        : config_(std::move(other.config_))
        , socketcan_fd_(other.socketcan_fd_) {

        // Take ownership of socket FD
        other.socketcan_fd_ = -1;  // Invalidate source
    }

    SocketCANBridge& SocketCANBridge::operator=(SocketCANBridge&& other) noexcept {
        if (this != &other) {
            // Close our current socket if open
            try {
                if (socketcan_fd_ >= 0) {
                    close_socketcan_socket();
                }
            } catch (...) {
                // Ignore errors during cleanup
            }

            // Move data from other
            config_ = std::move(other.config_);
            socketcan_fd_ = other.socketcan_fd_;

            // Invalidate source
            other.socketcan_fd_ = -1;
        }
        return *this;
    }

    // === SocketCAN Socket Management ===

    int SocketCANBridge::open_socketcan_socket(const std::string& interface) {
        // Create SocketCAN socket
        int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (fd < 0) {
            throw DeviceException(
                Status::DCONFIG_ERROR,
                "Failed to create SocketCAN socket: " + std::string(strerror(errno))
            );
        }

        // Get interface index
        struct ifreq ifr;
        std::memset(&ifr, 0, sizeof(ifr));
        std::strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';  // Ensure null termination

        if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
            close(fd);  // Clean up socket before throwing
            throw DeviceException(
                Status::DNOT_FOUND,
                "SocketCAN interface '" + interface + "' not found: " +
                std::string(strerror(errno))
            );
        }

        // Bind socket to CAN interface
        struct sockaddr_can addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        if (bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
            close(fd);  // Clean up socket before throwing
            throw DeviceException(
                Status::DCONFIG_ERROR,
                "Failed to bind SocketCAN socket to '" + interface + "': " +
                std::string(strerror(errno))
            );
        }

        return fd;
    }

    void SocketCANBridge::set_socketcan_timeouts() {
        if (socketcan_fd_ < 0) {
            throw DeviceException(
                Status::DNOT_OPEN,
                "Cannot set timeouts on closed SocketCAN socket"
            );
        }

        // Convert milliseconds to struct timeval
        struct timeval tv;
        tv.tv_sec = config_.socketcan_read_timeout_ms / 1000;
        tv.tv_usec = (config_.socketcan_read_timeout_ms % 1000) * 1000;

        // Set receive timeout
        if (setsockopt(socketcan_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            throw DeviceException(
                Status::DCONFIG_ERROR,
                "Failed to set SocketCAN receive timeout: " +
                std::string(strerror(errno))
            );
        }
    }

    void SocketCANBridge::close_socketcan_socket() {
        if (socketcan_fd_ < 0) {
            return;  // Already closed
        }

        if (close(socketcan_fd_) < 0) {
            int saved_errno = errno;
            socketcan_fd_ = -1;  // Mark as closed even on error
            throw DeviceException(
                Status::DCONFIG_ERROR,
                "Failed to close SocketCAN socket: " +
                std::string(strerror(saved_errno))
            );
        }

        socketcan_fd_ = -1;
    }

    // === USB Adapter Management ===

    void SocketCANBridge::initialize_usb_adapter() {
        try {
            // Create USBAdapter (constructor auto-opens and configures port)
            adapter_ = std::make_shared<USBAdapter>(
                config_.usb_device_path,
                config_.serial_baud_rate
            );
        } catch (const std::exception& e) {
            throw DeviceException(
                Status::DCONFIG_ERROR,
                "Failed to initialize USB adapter: " + std::string(e.what())
            );
        }
    }

    void SocketCANBridge::configure_usb_adapter() {
        if (!adapter_) {
            throw DeviceException(
                Status::DNOT_OPEN,
                "USB adapter not initialized"
            );
        }

        // Determine CANVersion based on whether we're using extended IDs
        // For now, use standard unless filter requires extended
        CANVersion can_version = CANVersion::STD_FIXED;
        if (config_.filter_id > 0x7FF || config_.filter_mask > 0x7FF) {
            can_version = CANVersion::EXT_FIXED;
        }

        // Convert bool to RTX enum
        RTX rtx_mode = config_.auto_retransmit ? RTX::AUTO : RTX::OFF;

        // Build ConfigFrame
        auto config_frame = FrameBuilder<ConfigFrame>()
            .with_can_version(can_version)
            .with_baud_rate(config_.can_baud_rate)
            .with_mode(config_.can_mode)
            .with_rtx(rtx_mode)
            .with_filter(config_.filter_id)
            .with_mask(config_.filter_mask)
            .build();

        // Send configuration to adapter
        try {
            adapter_->send_frame(config_frame);
        } catch (const std::exception& e) {
            throw DeviceException(
                Status::DWRITE_ERROR,
                "Failed to send config to USB adapter: " + std::string(e.what())
            );
        }

        // Verify configuration (optional - could read back and validate)
        // For now, we trust the send was successful
        // Future: Read back ConfigFrame and verify
    }

    void SocketCANBridge::verify_adapter_config() {
        // Placeholder for future implementation
        // Would read back ConfigFrame and validate:
        // 1. Read ConfigFrame from adapter
        // 2. Compare baud rate, mode, filter, mask
        // 3. Throw if mismatch

        // Note: Current USBAdapter doesn't have a "read config" method
        // This would require protocol extension or adapter query capability
    }

    // === Statistics Methods ===

    BridgeStatisticsSnapshot SocketCANBridge::get_statistics() const {
        BridgeStatisticsSnapshot snapshot;

        // Copy all atomic values to non-atomic struct
        // Using relaxed memory order since we just need a snapshot, not strict ordering
        snapshot.usb_rx_frames = stats_.usb_rx_frames.load(std::memory_order_relaxed);
        snapshot.usb_tx_frames = stats_.usb_tx_frames.load(std::memory_order_relaxed);
        snapshot.socketcan_rx_frames = stats_.socketcan_rx_frames.load(std::memory_order_relaxed);
        snapshot.socketcan_tx_frames = stats_.socketcan_tx_frames.load(std::memory_order_relaxed);
        snapshot.usb_rx_errors = stats_.usb_rx_errors.load(std::memory_order_relaxed);
        snapshot.usb_tx_errors = stats_.usb_tx_errors.load(std::memory_order_relaxed);
        snapshot.socketcan_rx_errors = stats_.socketcan_rx_errors.load(std::memory_order_relaxed);
        snapshot.socketcan_tx_errors = stats_.socketcan_tx_errors.load(std::memory_order_relaxed);
        snapshot.conversion_errors = stats_.conversion_errors.load(std::memory_order_relaxed);

        return snapshot;
    }

    void SocketCANBridge::reset_statistics() {
        stats_.reset();
    }

    // === Bridge Lifecycle Management ===

    void SocketCANBridge::start() {
        // Check if already running
        bool expected = false;
        if (!running_.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
            throw std::logic_error("Bridge is already running");
        }

        // Spawn forwarding threads
        usb_to_socketcan_thread_ = std::thread(&SocketCANBridge::usb_to_socketcan_loop, this);
        socketcan_to_usb_thread_ = std::thread(&SocketCANBridge::socketcan_to_usb_loop, this);
    }

    void SocketCANBridge::stop() {
        // Check if running
        if (!running_.load(std::memory_order_relaxed)) {
            return; // Already stopped
        }

        // Signal threads to stop
        running_.store(false, std::memory_order_relaxed);

        // Join threads
        if (usb_to_socketcan_thread_.joinable()) {
            usb_to_socketcan_thread_.join();
        }
        if (socketcan_to_usb_thread_.joinable()) {
            socketcan_to_usb_thread_.join();
        }
    }

    // === Forwarding Threads ===

    void SocketCANBridge::usb_to_socketcan_loop() {
        while (running_.load(std::memory_order_relaxed)) {
            try {
                // Read frame from USB adapter (with timeout)
                auto frame = adapter_->receive_variable_frame(config_.usb_read_timeout_ms);
                stats_.usb_rx_frames.fetch_add(1, std::memory_order_relaxed);

                // Convert to SocketCAN format
                struct can_frame cf = SocketCANHelper::to_socketcan(frame);

                // Write to SocketCAN socket
                ssize_t bytes = write(socketcan_fd_, &cf, sizeof(struct can_frame));
                if (bytes != sizeof(struct can_frame)) {
                    stats_.socketcan_tx_errors.fetch_add(1, std::memory_order_relaxed);
                    if (bytes < 0) {
                        std::cerr << "[USB→CAN] Socket write error: " << std::strerror(errno) <<
                            std::endl;
                    } else {
                        std::cerr << "[USB→CAN] Partial write: " << bytes << " bytes" << std::endl;
                    }
                } else {
                    stats_.socketcan_tx_frames.fetch_add(1, std::memory_order_relaxed);

                    // Invoke callback if set
                    if (usb_to_socketcan_callback_) {
                        usb_to_socketcan_callback_(frame, cf);
                    }
                }

            } catch (const TimeoutException&) {
                // Expected - USB read timeout allows checking running_ flag
                continue;
            }
            catch (const ProtocolException& e) {
                stats_.conversion_errors.fetch_add(1, std::memory_order_relaxed);
                std::cerr << "[USB→CAN] Conversion error: " << e.what() << std::endl;
            }
            catch (const std::exception& e) {
                stats_.usb_rx_errors.fetch_add(1, std::memory_order_relaxed);
                std::cerr << "[USB→CAN] USB RX error: " << e.what() << std::endl;
            }
        }
    }

    void SocketCANBridge::socketcan_to_usb_loop() {
        struct can_frame cf;
        fd_set readfds;
        struct timeval timeout;

        while (running_.load(std::memory_order_relaxed)) {
            try {
                // Set up select() for timeout handling
                FD_ZERO(&readfds);
                FD_SET(socketcan_fd_, &readfds);

                // Configure timeout
                timeout.tv_sec = config_.socketcan_read_timeout_ms / 1000;
                timeout.tv_usec = (config_.socketcan_read_timeout_ms % 1000) * 1000;

                // Wait for socket to be readable (with timeout)
                int ret = select(socketcan_fd_ + 1, &readfds, nullptr, nullptr, &timeout);

                if (ret < 0) {
                    // Select error
                    stats_.socketcan_rx_errors.fetch_add(1, std::memory_order_relaxed);
                    std::cerr << "[CAN→USB] select() error: " << std::strerror(errno) << std::endl;
                    continue;
                } else if (ret == 0) {
                    // Timeout - continue to check running_ flag
                    continue;
                }

                // Socket is readable - read CAN frame
                ssize_t bytes = read(socketcan_fd_, &cf, sizeof(struct can_frame));
                if (bytes < 0) {
                    stats_.socketcan_rx_errors.fetch_add(1, std::memory_order_relaxed);
                    std::cerr << "[CAN→USB] Socket read error: " << std::strerror(errno) <<
                        std::endl;
                    continue;
                } else if (bytes != sizeof(struct can_frame)) {
                    stats_.socketcan_rx_errors.fetch_add(1, std::memory_order_relaxed);
                    std::cerr << "[CAN→USB] Partial read: " << bytes << " bytes" << std::endl;
                    continue;
                }

                stats_.socketcan_rx_frames.fetch_add(1, std::memory_order_relaxed);

                // Convert to Waveshare VariableFrame
                auto frame = SocketCANHelper::from_socketcan(cf);

                // Send to USB adapter
                adapter_->send_frame(frame);
                stats_.usb_tx_frames.fetch_add(1, std::memory_order_relaxed);

                // Invoke callback if set
                if (socketcan_to_usb_callback_) {
                    socketcan_to_usb_callback_(cf, frame);
                }

            } catch (const ProtocolException& e) {
                stats_.conversion_errors.fetch_add(1, std::memory_order_relaxed);
                std::cerr << "[CAN→USB] Conversion error: " << e.what() << std::endl;
            }
            catch (const std::exception& e) {
                stats_.usb_tx_errors.fetch_add(1, std::memory_order_relaxed);
                std::cerr << "[CAN→USB] USB TX error: " << e.what() << std::endl;
            }
        }
    }

} // namespace waveshare


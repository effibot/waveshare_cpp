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
#include "../include/io/real_can_socket.hpp"

namespace waveshare {

    // === Constructor & Factory ===

    SocketCANBridge::SocketCANBridge(const BridgeConfig& config,
        std::unique_ptr<ICANSocket> can_socket,
        std::unique_ptr<USBAdapter> usb_adapter)
        : config_(config)
        , can_socket_(std::move(can_socket))
        , adapter_(std::move(usb_adapter)) {

        // Validate configuration
        config_.validate();

        // Validate injected dependencies
        if (!can_socket_ || !can_socket_->is_open()) {
            throw DeviceException(Status::DNOT_OPEN, "SocketCANBridge: CAN socket not open");
        }
        if (!adapter_ || !adapter_->is_open()) {
            throw DeviceException(Status::DNOT_OPEN, "SocketCANBridge: USB adapter not open");
        }

        // Configure USB adapter with CAN settings
        configure_usb_adapter();
    }

    std::unique_ptr<SocketCANBridge> SocketCANBridge::create(const BridgeConfig& config) {
        // Validate configuration first
        config.validate();

        // Create real CAN socket (auto-opens and configures)
        auto can_socket = std::make_unique<RealCANSocket>(
            config.socketcan_interface,
            config.socketcan_read_timeout_ms
        );

        // Create USB adapter (auto-opens and configures)
        auto usb_adapter = USBAdapter::create(
            config.usb_device_path,
            config.serial_baud_rate
        );

        // Configure USB adapter for CAN
        ConfigFrame can_config = FrameBuilder<ConfigFrame>()
            .with_baud_rate(config.can_baud_rate)
            .with_mode(config.can_mode)
            .with_filter(config.filter_id)
            .with_mask(config.filter_mask)
            .build();

        usb_adapter->send_frame(can_config);

        // Create bridge with injected dependencies
        return std::unique_ptr<SocketCANBridge>(
            new SocketCANBridge(config, std::move(can_socket), std::move(usb_adapter))
        );
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

        // CAN socket automatically closed by ICANSocket destructor
    }

    // === Move Operations (default, handles unique_ptr members correctly) ===
    // Move constructor and assignment use compiler-generated defaults

    // === USB Adapter Configuration ===

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

                // Write to CAN socket
                ssize_t bytes = can_socket_->send(cf);
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
        int can_fd = can_socket_->get_fd();

        while (running_.load(std::memory_order_relaxed)) {
            try {
                // Set up select() for timeout handling
                FD_ZERO(&readfds);
                FD_SET(can_fd, &readfds);

                // Configure timeout
                timeout.tv_sec = config_.socketcan_read_timeout_ms / 1000;
                timeout.tv_usec = (config_.socketcan_read_timeout_ms % 1000) * 1000;

                // Wait for socket to be readable (with timeout)
                int ret = select(can_fd + 1, &readfds, nullptr, nullptr, &timeout);

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
                ssize_t bytes = can_socket_->receive(cf);
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


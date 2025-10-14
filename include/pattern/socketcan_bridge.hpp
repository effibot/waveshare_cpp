/**
 * @file socketcan_bridge.hpp
 * @brief SocketCAN bridge for Waveshare USB-CAN adapter
 * @version 1.0
 * @date 2025-10-14
 *
 * Bridges Waveshare USB-CAN adapter protocol to Linux SocketCAN interface.
 * Provides bidirectional frame translation and forwarding between:
 * - USB serial (Waveshare protocol) ↔ SocketCAN (Linux CAN)
 *
 * @copyright Copyright (c) 2025
 */

#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <sstream>
#include <iomanip>

#include "bridge_config.hpp"
#include "usb_adapter.hpp"
#include "../frame/config_frame.hpp"

namespace waveshare {

    /**
     * @brief Statistics for bridge performance monitoring
     *
     * All counters are atomic for thread-safe updates.
     * Use get_statistics() to get a non-atomic snapshot.
     */
    struct BridgeStatistics {
        std::atomic<uint64_t> usb_rx_frames{0};        ///< Frames received from USB
        std::atomic<uint64_t> usb_tx_frames{0};        ///< Frames sent to USB
        std::atomic<uint64_t> socketcan_rx_frames{0};  ///< Frames received from SocketCAN
        std::atomic<uint64_t> socketcan_tx_frames{0};  ///< Frames sent to SocketCAN
        std::atomic<uint64_t> usb_rx_errors{0};        ///< USB receive errors
        std::atomic<uint64_t> usb_tx_errors{0};        ///< USB send errors
        std::atomic<uint64_t> socketcan_rx_errors{0};  ///< SocketCAN receive errors
        std::atomic<uint64_t> socketcan_tx_errors{0};  ///< SocketCAN send errors
        std::atomic<uint64_t> conversion_errors{0};    ///< Frame conversion failures

        /**
         * @brief Reset all counters to zero
         */
        void reset() {
            usb_rx_frames.store(0, std::memory_order_relaxed);
            usb_tx_frames.store(0, std::memory_order_relaxed);
            socketcan_rx_frames.store(0, std::memory_order_relaxed);
            socketcan_tx_frames.store(0, std::memory_order_relaxed);
            usb_rx_errors.store(0, std::memory_order_relaxed);
            usb_tx_errors.store(0, std::memory_order_relaxed);
            socketcan_rx_errors.store(0, std::memory_order_relaxed);
            socketcan_tx_errors.store(0, std::memory_order_relaxed);
            conversion_errors.store(0, std::memory_order_relaxed);
        }

        /**
         * @brief Get human-readable statistics string
         * @return std::string Formatted statistics
         */
        std::string to_string() const {
            std::ostringstream oss;
            oss << "Bridge Statistics:\n"
                << "  USB RX:        " << std::setw(10) <<
                usb_rx_frames.load(std::memory_order_relaxed) << " frames\n"
                << "  USB TX:        " << std::setw(10) <<
                usb_tx_frames.load(std::memory_order_relaxed) << " frames\n"
                << "  SocketCAN RX:  " << std::setw(10) <<
                socketcan_rx_frames.load(std::memory_order_relaxed) << " frames\n"
                << "  SocketCAN TX:  " << std::setw(10) <<
                socketcan_tx_frames.load(std::memory_order_relaxed) << " frames\n"
                << "  USB RX Errors: " << std::setw(10) <<
                usb_rx_errors.load(std::memory_order_relaxed) << "\n"
                << "  USB TX Errors: " << std::setw(10) <<
                usb_tx_errors.load(std::memory_order_relaxed) << "\n"
                << "  CAN RX Errors: " << std::setw(10) <<
                socketcan_rx_errors.load(std::memory_order_relaxed) << "\n"
                << "  CAN TX Errors: " << std::setw(10) <<
                socketcan_tx_errors.load(std::memory_order_relaxed) << "\n"
                << "  Conv Errors:   " << std::setw(10) <<
                conversion_errors.load(std::memory_order_relaxed);
            return oss.str();
        }
    };

    /**
     * @brief Non-atomic snapshot of bridge statistics
     *
     * Used to return statistics values without atomic overhead.
     */
    struct BridgeStatisticsSnapshot {
        uint64_t usb_rx_frames;
        uint64_t usb_tx_frames;
        uint64_t socketcan_rx_frames;
        uint64_t socketcan_tx_frames;
        uint64_t usb_rx_errors;
        uint64_t usb_tx_errors;
        uint64_t socketcan_rx_errors;
        uint64_t socketcan_tx_errors;
        uint64_t conversion_errors;

        /**
         * @brief Get human-readable statistics string
         * @return std::string Formatted statistics
         */
        std::string to_string() const {
            std::ostringstream oss;
            oss << "Bridge Statistics Snapshot:\n"
                << "  USB RX:        " << std::setw(10) << usb_rx_frames << " frames\n"
                << "  USB TX:        " << std::setw(10) << usb_tx_frames << " frames\n"
                << "  SocketCAN RX:  " << std::setw(10) << socketcan_rx_frames << " frames\n"
                << "  SocketCAN TX:  " << std::setw(10) << socketcan_tx_frames << " frames\n"
                << "  USB RX Errors: " << std::setw(10) << usb_rx_errors << "\n"
                << "  USB TX Errors: " << std::setw(10) << usb_tx_errors << "\n"
                << "  CAN RX Errors: " << std::setw(10) << socketcan_rx_errors << "\n"
                << "  CAN TX Errors: " << std::setw(10) << socketcan_tx_errors << "\n"
                << "  Conv Errors:   " << std::setw(10) << conversion_errors;
            return oss.str();
        }
    };

    /**
     * @brief SocketCAN bridge for Waveshare USB-CAN adapter
     *
     * Architecture:
     * - Manages SocketCAN socket lifecycle
     * - Integrates USBAdapter for serial communication
     * - Bidirectional frame forwarding (future phases)
     * - Thread-safe operation with proper cleanup
     */
    class SocketCANBridge {
        public:
            /**
             * @brief Construct bridge with configuration
             * @param config Bridge configuration (validated on construction)
             * @throws DeviceException if USB device or SocketCAN interface unavailable
             * @throws std::invalid_argument if config is invalid
             */
            explicit SocketCANBridge(const BridgeConfig& config);

            /**
             * @brief Destructor - ensures clean shutdown
             * Closes SocketCAN socket and stops any active threads
             */
            ~SocketCANBridge();

            // Delete copy operations (bridge owns resources)
            SocketCANBridge(const SocketCANBridge&) = delete;
            SocketCANBridge& operator=(const SocketCANBridge&) = delete;

            // Custom move operations to properly handle socket FD
            SocketCANBridge(SocketCANBridge&& other) noexcept;
            SocketCANBridge& operator=(SocketCANBridge&& other) noexcept;

            /**
             * @brief Get current configuration
             * @return const BridgeConfig& Configuration reference
             */
            const BridgeConfig& get_config() const { return config_; }

            /**
             * @brief Check if SocketCAN socket is open
             * @return bool True if socket is open
             */
            bool is_socketcan_open() const { return socketcan_fd_ >= 0; }

            /**
             * @brief Get SocketCAN file descriptor
             * @return int File descriptor or -1 if closed
             */
            int get_socketcan_fd() const { return socketcan_fd_; }

            /**
             * @brief Get USB adapter
             * @return std::shared_ptr<USBAdapter> Adapter pointer (may be null)
             */
            std::shared_ptr<USBAdapter> get_adapter() const { return adapter_; }

            /**
             * @brief Get statistics snapshot
             * @return BridgeStatisticsSnapshot Non-atomic copy of current statistics
             */
            BridgeStatisticsSnapshot get_statistics() const;

            /**
             * @brief Reset all statistics counters to zero
             */
            void reset_statistics();

        private:
            // === Configuration ===
            BridgeConfig config_;

            // === SocketCAN Socket Management ===
            int socketcan_fd_ = -1;

            // === USB Adapter Management ===
            std::shared_ptr<USBAdapter> adapter_;

            // === Statistics ===
            BridgeStatistics stats_;

            /**
             * @brief Initialize and configure USB adapter
             * @throws DeviceException on adapter initialization failure
             */
            void initialize_usb_adapter();

            /**
             * @brief Configure USB adapter with CAN parameters
             * @throws DeviceException on configuration failure
             */
            void configure_usb_adapter();

            /**
             * @brief Verify adapter configuration
             * Reads back configuration and validates it matches
             * @throws DeviceException on verification failure
             */
            void verify_adapter_config();

            /**
             * @brief Open SocketCAN socket and bind to interface
             * @param interface SocketCAN interface name (e.g., "can0", "vcan0")
             * @return int Socket file descriptor
             * @throws DeviceException on socket creation, bind, or ioctl failure
             */
            int open_socketcan_socket(const std::string& interface);

            /**
             * @brief Set receive timeout on SocketCAN socket
             * @throws DeviceException on setsockopt failure
             */
            void set_socketcan_timeouts();

            /**
             * @brief Close SocketCAN socket
             * @throws DeviceException on close failure
             */
            void close_socketcan_socket();
    };

} // namespace waveshare

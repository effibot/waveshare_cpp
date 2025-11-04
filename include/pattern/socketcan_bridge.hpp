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
#include <functional>
#include <linux/can.h>

#include "bridge_config.hpp"
#include "usb_adapter.hpp"
#include "../io/can_socket.hpp"
#include "../frame/config_frame.hpp"
#include "../frame/variable_frame.hpp"

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
     * @brief Lock-free bidirectional bridge between Waveshare USB-CAN and Linux SocketCAN
     *
     * Provides real-time frame forwarding between Waveshare USB-CAN adapters and
     * Linux SocketCAN interfaces with protocol conversion and performance monitoring.
     *
     * ## Architecture
     *
     * The bridge operates with **two independent forwarding threads**:
     * - **USB → SocketCAN**: Reads Waveshare frames, converts to can_frame, writes to SocketCAN
     * - **SocketCAN → USB**: Reads can_frame, converts to Waveshare frames, writes to USB
     *
     * @code{.cpp}
     * ┌──────────────────────────────────────────────────┐
     * │          SocketCANBridge                         │
     * │                                                  │
     * │  Thread 1: USB → CAN     Thread 2: CAN → USB    │
     * │  ┌──────────────────┐    ┌──────────────────┐  │
     * │  │ USB Adapter RX   │    │ SocketCAN RX     │  │
     * │  │ Frame Conversion │    │ Frame Conversion │  │
     * │  │ SocketCAN TX     │    │ USB Adapter TX   │  │
     * │  └──────────────────┘    └──────────────────┘  │
     * │                                                  │
     * │  BridgeStatistics (lock-free atomic counters)   │
     * └──────────────────────────────────────────────────┘
     * @endcode
     *
     * ## Thread Safety
     *
     * Uses **lock-free synchronization** for maximum performance:
     * - **running_** (atomic<bool>): Controls thread lifecycle
     * - **All statistics** (atomic<uint64_t>): Lock-free performance counters
     * - **callback_mutex_**: Only protects callback registration (not invocation)
     *
     * ### Deadlock Prevention
     * No deadlocks possible due to:
     * 1. **Independent threads**: No inter-thread waiting or synchronization
     * 2. **Lock-free statistics**: No contention on performance counters
     * 3. **Timeout-based I/O**: All blocking operations have configurable timeouts
     * 4. **Unidirectional flow**: Each thread owns one data flow direction
     *
     * ### Memory Ordering
     * - Statistics use `std::memory_order_relaxed` for performance
     * - Control flags use sequential consistency for correctness
     *
     * ## Dependency Injection
     *
     * Supports testing without hardware:
     * @code{.cpp}
     * // Production: Real hardware
     * auto bridge = SocketCANBridge::create(config);
     *
     * // Testing: Mock hardware
     * auto mock_socket = std::make_unique<MockCANSocket>();
     * auto mock_adapter = std::make_unique<USBAdapter>(...);
     * auto bridge = std::make_unique<SocketCANBridge>(
     *     config, std::move(mock_socket), std::move(mock_adapter)
     * );
     * @endcode
     *
     * ## Features
     * - Bidirectional frame conversion (Waveshare VariableFrame ↔ can_frame)
     * - Dual-threaded concurrent forwarding
     * - Lock-free performance statistics
     * - Configurable timeouts, filters, and error handling
     * - Optional frame-level callbacks for monitoring
     * - Clean lifecycle management (start/stop/destructor)
     *
     * @see doc/SYNCHRONIZATION.md for detailed threading analysis
     * @see BridgeConfig for configuration options
     * @note All public methods are thread-safe
     */
    class SocketCANBridge {
        public:
            /**
             * @brief Constructor with dependency injection
             * @param config Bridge configuration
             * @param can_socket Injected CAN socket (real or mock)
             * @param usb_adapter Injected USB adapter
             * @throws std::invalid_argument if config is invalid
             */
            SocketCANBridge(const BridgeConfig& config,
                std::unique_ptr<ICANSocket> can_socket,
                std::unique_ptr<USBAdapter> usb_adapter);

            /**
             * @brief Factory method to create bridge with real hardware
             * @param config Bridge configuration
             * @return std::unique_ptr<SocketCANBridge> Configured bridge ready to use
             * @throws DeviceException if USB device or SocketCAN interface unavailable
             * @throws std::invalid_argument if config is invalid
             */
            static std::unique_ptr<SocketCANBridge> create(const BridgeConfig& config);

            /**
             * @brief Destructor - ensures clean shutdown
             * Closes SocketCAN socket and stops any active threads
             */
            ~SocketCANBridge();

            // Delete copy operations (bridge owns resources)
            SocketCANBridge(const SocketCANBridge&) = delete;
            SocketCANBridge& operator=(const SocketCANBridge&) = delete;

            // Delete move operations (BridgeStatistics has atomic members that can't be moved)
            SocketCANBridge(SocketCANBridge&& other) noexcept = delete;
            SocketCANBridge& operator=(SocketCANBridge&& other) noexcept = delete;

            /**
             * @brief Get current configuration
             * @return const BridgeConfig& Configuration reference
             */
            const BridgeConfig& get_config() const { return config_; }

            /**
             * @brief Check if the underlying USB port is open
             * @return bool True if USB port is open
             */
            bool is_usb_open() const {
                return adapter_ && adapter_->is_open();
            }

            /**
             * @brief Check if SocketCAN socket is open
             * @return bool True if socket is open
             */
            bool is_socketcan_open() const {
                return can_socket_ && can_socket_->is_open();
            }

            /**
             * @brief Get SocketCAN file descriptor
             * @return int File descriptor or -1 if closed
             */
            int get_socketcan_fd() const {
                return can_socket_ ? can_socket_->get_fd() : -1;
            }

            /**
             * @brief Get USB adapter (non-owning pointer)
             * @return USBAdapter* Adapter pointer (may be null)
             */
            USBAdapter* get_adapter() const {
                return adapter_.get();
            }

            /**
             * @brief Get statistics snapshot
             * @return BridgeStatisticsSnapshot Non-atomic copy of current statistics
             */
            BridgeStatisticsSnapshot get_statistics() const;

            /**
             * @brief Reset all statistics counters to zero
             */
            void reset_statistics();

            /**
             * @brief Start the bridge forwarding threads
             * @throws std::logic_error if bridge is already running
             */
            void start();

            /**
             * @brief Stop the bridge forwarding threads
             * Blocks until both threads have joined
             */
            void stop();

            /**
             * @brief Check if bridge is currently running
             * @return bool True if forwarding threads are active
             */
            bool is_running() const { return running_.load(std::memory_order_relaxed); }

            /**
             * @brief Set callback for USB → SocketCAN frame forwarding
             * @param callback Function called with (VariableFrame, can_frame) when frame is forwarded
             *
             * Callback signature: void(const VariableFrame& usb_frame, const ::can_frame& socketcan_frame)
             * Called from USB→SocketCAN thread, should be thread-safe and non-blocking.
             */
            void set_usb_to_socketcan_callback(std::function<void(const VariableFrame&,
                const ::can_frame&)> callback) {
                usb_to_socketcan_callback_ = std::move(callback);
            }

            /**
             * @brief Set callback for SocketCAN → USB frame forwarding
             * @param callback Function called with (can_frame, VariableFrame) when frame is forwarded
             *
             * Callback signature: void(const ::can_frame& socketcan_frame, const VariableFrame& usb_frame)
             * Called from SocketCAN→USB thread, should be thread-safe and non-blocking.
             */
            void set_socketcan_to_usb_callback(std::function<void(const ::can_frame&,
                const VariableFrame&)> callback) {
                socketcan_to_usb_callback_ = std::move(callback);
            }

        private:
            // === Configuration ===
            BridgeConfig config_;

            // === I/O Abstractions ===
            std::unique_ptr<ICANSocket> can_socket_;  // Injected CAN socket (real or mock)
            std::unique_ptr<USBAdapter> adapter_;     // USB adapter

            // === Statistics ===
            BridgeStatistics stats_;

            // === Threading ===
            std::atomic<bool> running_{false};
            std::thread usb_to_socketcan_thread_;
            std::thread socketcan_to_usb_thread_;

            // === Callbacks ===
            std::function<void(const VariableFrame&,
                const ::can_frame&)> usb_to_socketcan_callback_;
            std::function<void(const ::can_frame&,
                const VariableFrame&)> socketcan_to_usb_callback_;

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

            // === Socket management methods (removed - now in RealCANSocket) ===

            /**
             * @brief USB to SocketCAN forwarding loop (runs in thread)
             *
             * Continuously reads frames from USB adapter and forwards to SocketCAN socket.
             * Runs while running_ flag is true. Handles timeouts and errors gracefully.
             */
            void usb_to_socketcan_loop();

            /**
             * @brief SocketCAN to USB forwarding loop (runs in thread)
             *
             * Continuously reads frames from SocketCAN socket and forwards to USB adapter.
             * Uses select() for timeout handling. Runs while running_ flag is true.
             */
            void socketcan_to_usb_loop();
    };

} // namespace waveshare


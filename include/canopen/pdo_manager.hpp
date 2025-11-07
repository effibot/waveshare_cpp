/**
 * @file pdo_manager.hpp
 * @brief PDO (Process Data Object) Manager for multi-motor CANopen communication
 * @author effibot (andrea.efficace1@gmail.com)
 * @date 2025-11-07
 *
 * Manages PDO communication for multiple motors on a single CAN bus:
 * - Routes TPDOs (feedback) from motors to appropriate callbacks
 * - Sends RPDOs (commands) to motors with correct COB-IDs
 * - Single receive thread for all motors (efficient)
 * - Thread-safe operation
 */

#pragma once

#include "canopen/pdo_constants.hpp"
#include <linux/can.h>
#include <linux/can/raw.h>
#include <functional>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>
#include <string>
#include <cstdint>

namespace canopen {

/**
 * @class PDOManager
 * @brief Manages PDO communication for multiple CANopen motors
 *
 * The PDO Manager handles real-time cyclic communication with multiple motors:
 * - RPDO (Receive PDO): Commands FROM host TO motor
 * - TPDO (Transmit PDO): Feedback FROM motor TO host
 *
 * CANopen PDO COB-IDs (standard mapping):
 * - RPDO1: 0x200 + node_id (command, typically position/velocity)
 * - RPDO2: 0x300 + node_id (additional commands)
 * - TPDO1: 0x180 + node_id (feedback, typically statusword + position)
 * - TPDO2: 0x280 + node_id (additional feedback, typically velocity)
 */
    class PDOManager {
        public:
            /**
             * @brief Construct PDO Manager
             * @param can_interface CAN interface name (e.g., "vcan0", "can0")
             */
            explicit PDOManager(const std::string& can_interface);

            /**
             * @brief Destructor - stops receive thread and closes socket
             */
            ~PDOManager();

            // Prevent copying
            PDOManager(const PDOManager&) = delete;
            PDOManager& operator=(const PDOManager&) = delete;

            // =========================================================================
            // Lifecycle Management
            // =========================================================================

            /**
             * @brief Start PDO communication (open socket, start receive thread)
             * @return true if successful, false otherwise
             */
            bool start();

            /**
             * @brief Stop PDO communication (stop thread, close socket)
             */
            void stop();

            /**
             * @brief Check if PDO manager is running
             * @return true if receive thread is active
             */
            bool is_running() const { return running_.load(); }

            // =========================================================================
            // RPDO Sending (Commands to Motors)
            // =========================================================================

            /**
             * @brief Send RPDO1 to motor
             * @param node_id CANopen node ID (1-127)
             * @param data PDO data payload (up to 8 bytes)
             * @return true if sent successfully
             *
             * Typical RPDO1 mapping for CIA402:
             * - Controlword (2 bytes)
             * - Target Position (4 bytes)
             * - or Target Velocity (4 bytes)
             */
            bool send_rpdo1(uint8_t node_id, const std::vector<uint8_t>& data);

            /**
             * @brief Send RPDO2 to motor
             * @param node_id CANopen node ID (1-127)
             * @param data PDO data payload (up to 8 bytes)
             * @return true if sent successfully
             *
             * Typical RPDO2 mapping for CIA402:
             * - Additional parameters (e.g., acceleration)
             */
            bool send_rpdo2(uint8_t node_id, const std::vector<uint8_t>& data);

            // =========================================================================
            // TPDO Reception (Feedback from Motors)
            // =========================================================================

            /**
             * @brief Callback type for TPDO reception
             * @param frame Raw CAN frame received
             */
            using TPDOCallback = std::function<void (const can_frame&)>;

            /**
             * @brief Register callback for TPDO1 from specific motor
             * @param node_id CANopen node ID (1-127)
             * @param callback Function to call when TPDO1 received
             *
             * Typical TPDO1 mapping for CIA402:
             * - Statusword (2 bytes)
             * - Position Actual Value (4 bytes)
             */
            void register_tpdo1_callback(uint8_t node_id, TPDOCallback callback);

            /**
             * @brief Register callback for TPDO2 from specific motor
             * @param node_id CANopen node ID (1-127)
             * @param callback Function to call when TPDO2 received
             *
             * Typical TPDO2 mapping for CIA402:
             * - Velocity Actual Value (4 bytes)
             * - Current Actual Value (2 bytes)
             */
            void register_tpdo2_callback(uint8_t node_id, TPDOCallback callback);

            /**
             * @brief Unregister all callbacks for specific motor
             * @param node_id CANopen node ID
             */
            void unregister_callbacks(uint8_t node_id);

            // =========================================================================
            // Statistics & Diagnostics
            // =========================================================================

            /**
             * @brief PDO communication statistics with atomic counters
             *
             * All counters are atomic for lock-free thread-safe updates.
             * Use get_statistics() to get a non-atomic snapshot.
             */
            struct Statistics {
                std::atomic<uint64_t> tpdo1_received{0};
                std::atomic<uint64_t> tpdo2_received{0};
                std::atomic<uint64_t> rpdo1_sent{0};
                std::atomic<uint64_t> rpdo2_sent{0};
                std::atomic<uint64_t> errors{0};
                std::atomic<uint64_t> total_latency_us{0}; // Sum for average calculation
                std::atomic<uint64_t> latency_samples{0}; // Count for average calculation
                std::chrono::steady_clock::time_point last_tpdo1_time;
                std::chrono::steady_clock::time_point last_tpdo2_time;

                /**
                 * @brief Reset all counters to zero
                 */
                void reset() {
                    tpdo1_received.store(0, std::memory_order_relaxed);
                    tpdo2_received.store(0, std::memory_order_relaxed);
                    rpdo1_sent.store(0, std::memory_order_relaxed);
                    rpdo2_sent.store(0, std::memory_order_relaxed);
                    errors.store(0, std::memory_order_relaxed);
                    total_latency_us.store(0, std::memory_order_relaxed);
                    latency_samples.store(0, std::memory_order_relaxed);
                }

                /**
                 * @brief Calculate average latency
                 * @return Average latency in microseconds, or 0.0 if no samples
                 */
                double get_avg_latency_us() const {
                    uint64_t samples = latency_samples.load(std::memory_order_relaxed);
                    if (samples == 0) return 0.0;
                    uint64_t total = total_latency_us.load(std::memory_order_relaxed);
                    return static_cast<double>(total) / static_cast<double>(samples);
                }
            };

            /**
             * @brief Non-atomic snapshot of PDO statistics
             *
             * Used to return statistics values without atomic overhead.
             */
            struct StatisticsSnapshot {
                uint64_t tpdo1_received;
                uint64_t tpdo2_received;
                uint64_t rpdo1_sent;
                uint64_t rpdo2_sent;
                uint64_t errors;
                double avg_latency_us;
                std::chrono::steady_clock::time_point last_tpdo1_time;
                std::chrono::steady_clock::time_point last_tpdo2_time;
            };

            /**
             * @brief Get communication statistics for specific motor
             * @param node_id CANopen node ID
             * @return Non-atomic snapshot of statistics
             */
            StatisticsSnapshot get_statistics(uint8_t node_id) const;

            /**
             * @brief Reset statistics for specific motor
             * @param node_id CANopen node ID
             */
            void reset_statistics(uint8_t node_id);

            /**
             * @brief Get CAN interface name
             * @return Interface name (e.g., "vcan0")
             */
            std::string get_interface() const { return can_interface_; }

        private:
            // =========================================================================
            // SocketCAN Communication
            // =========================================================================

            int socket_fd_;
            std::string can_interface_;

            // Receive thread
            std::thread receive_thread_;
            std::atomic<bool> running_;

            /**
             * @brief Receive loop running in separate thread
             * Continuously receives CAN frames and dispatches to callbacks
             */
            void receive_loop();

            /**
             * @brief Dispatch received TPDO to registered callback
             * @param frame CAN frame received
             */
            void dispatch_tpdo(const can_frame& frame);

            /**
             * @brief Send CAN frame
             * @param frame CAN frame to send
             * @return true if sent successfully
             */
            bool send_frame(const can_frame& frame);

            // =========================================================================
            // Callback Management
            // =========================================================================

            // Callback registry: COB-ID → callback function
            std::unordered_map<uint32_t, TPDOCallback> tpdo_callbacks_;
            mutable std::mutex callback_mutex_;

            // =========================================================================
            // Statistics Management
            // =========================================================================

            // Statistics per motor: node_id → stats
            // Mutex only protects map access, not the atomic counters within Statistics
            std::unordered_map<uint8_t, Statistics> stats_;
            mutable std::mutex stats_mutex_;

            // =========================================================================
            // COB-ID Calculation (CANopen Standard)
            // =========================================================================

            /**
             * @brief Calculate RPDO1 COB-ID for node
             * @param node_id CANopen node ID (1-127)
             * @return COB-ID (RPDO1_BASE + node_id)
             */
            static uint32_t rpdo1_cob_id(uint8_t node_id) {
                return pdo::cob_id::RPDO1_BASE + node_id;
            }

            /**
             * @brief Calculate RPDO2 COB-ID for node
             * @param node_id CANopen node ID (1-127)
             * @return COB-ID (RPDO2_BASE + node_id)
             */
            static uint32_t rpdo2_cob_id(uint8_t node_id) {
                return pdo::cob_id::RPDO2_BASE + node_id;
            }

            /**
             * @brief Calculate TPDO1 COB-ID for node
             * @param node_id CANopen node ID (1-127)
             * @return COB-ID (TPDO1_BASE + node_id)
             */
            static uint32_t tpdo1_cob_id(uint8_t node_id) {
                return pdo::cob_id::TPDO1_BASE + node_id;
            }

            /**
             * @brief Calculate TPDO2 COB-ID for node
             * @param node_id CANopen node ID (1-127)
             * @return COB-ID (TPDO2_BASE + node_id)
             */
            static uint32_t tpdo2_cob_id(uint8_t node_id) {
                return pdo::cob_id::TPDO2_BASE + node_id;
            }

            /**
             * @brief Extract node ID from COB-ID
             * @param cob_id COB-ID from CAN frame
             * @return Node ID, or 0 if not a valid PDO COB-ID
             */
            static uint8_t extract_node_id(uint32_t cob_id) {
                return pdo::extract_node_id(cob_id);
            }
    };

} // namespace canopen

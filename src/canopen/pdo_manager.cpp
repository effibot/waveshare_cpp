/**
 * @file pdo_manager.cpp
 * @brief PDO Manager implementation
 * @author effibot (andrea.efficace1@gmail.com)
 * @date 2025-11-07
 */

#include "canopen/pdo_manager.hpp"
#include "canopen/pdo_constants.hpp"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace canopen {

// =============================================================================
// Constructor / Destructor
// =============================================================================

    PDOManager::PDOManager(const std::string& can_interface)
        : socket_fd_(-1)
        , can_interface_(can_interface)
        , running_(false) {
    }

    PDOManager::~PDOManager() {
        stop();
    }

// =============================================================================
// Lifecycle Management
// =============================================================================

    bool PDOManager::start() {
        if (running_.load()) {
            std::cerr << "[PDO] Already running" << std::endl;
            return true;
        }

        // Create SocketCAN socket
        socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (socket_fd_ < 0) {
            std::cerr << "[PDO] Failed to create socket: " << strerror(errno) << std::endl;
            return false;
        }

        // Get interface index
        struct ifreq ifr;
        std::strncpy(ifr.ifr_name, can_interface_.c_str(), IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';

        if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
            std::cerr << "[PDO] Failed to get interface index for " << can_interface_
                      << ": " << strerror(errno) << std::endl;
            close(socket_fd_);
            socket_fd_ = -1;
            return false;
        }

        // Bind socket to interface
        struct sockaddr_can addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        if (bind(socket_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
            std::cerr << "[PDO] Failed to bind socket: " << strerror(errno) << std::endl;
            close(socket_fd_);
            socket_fd_ = -1;
            return false;
        }

        std::cout << "[PDO] Connected to " << can_interface_ << std::endl;

        // Start receive thread
        running_.store(true);
        receive_thread_ = std::thread(&PDOManager::receive_loop, this);

        return true;
    }

    void PDOManager::stop() {
        if (!running_.load()) {
            return;
        }

        std::cout << "[PDO] Stopping PDO manager..." << std::endl;

        // Stop receive thread
        running_.store(false);
        if (receive_thread_.joinable()) {
            receive_thread_.join();
        }

        // Close socket
        if (socket_fd_ >= 0) {
            close(socket_fd_);
            socket_fd_ = -1;
        }

        std::cout << "[PDO] Stopped" << std::endl;
    }

// =============================================================================
// RPDO Sending (Commands to Motors)
// =============================================================================

    bool PDOManager::send_rpdo1(uint8_t node_id, const std::vector<uint8_t>& data) {
        if (data.size() > pdo::limits::MAX_PDO_DATA_LENGTH) {
            std::cerr << "[PDO] RPDO1 data too large: " << data.size() << " bytes" << std::endl;
            return false;
        }

        // Construct CAN frame
        struct can_frame frame;
        std::memset(&frame, 0, sizeof(frame));
        frame.can_id = rpdo1_cob_id(node_id);
        frame.can_dlc = data.size();
        std::memcpy(frame.data, data.data(), data.size());

        bool success = send_frame(frame);

        if (success) {
            // Lock-free atomic increment
            stats_[node_id].rpdo1_sent.fetch_add(1, std::memory_order_relaxed);
        }

        return success;
    }

    bool PDOManager::send_rpdo2(uint8_t node_id, const std::vector<uint8_t>& data) {
        if (data.size() > pdo::limits::MAX_PDO_DATA_LENGTH) {
            std::cerr << "[PDO] RPDO2 data too large: " << data.size() << " bytes" << std::endl;
            return false;
        }

        // Construct CAN frame
        struct can_frame frame;
        std::memset(&frame, 0, sizeof(frame));
        frame.can_id = rpdo2_cob_id(node_id);
        frame.can_dlc = data.size();
        std::memcpy(frame.data, data.data(), data.size());

        bool success = send_frame(frame);

        if (success) {
            // Lock-free atomic increment
            stats_[node_id].rpdo2_sent.fetch_add(1, std::memory_order_relaxed);
        }

        return success;
    }

    bool PDOManager::send_frame(const can_frame& frame) {
        if (socket_fd_ < 0) {
            std::cerr << "[PDO] Socket not open" << std::endl;
            return false;
        }

        ssize_t nbytes = send(socket_fd_, &frame, sizeof(frame), 0);

        if (nbytes != sizeof(frame)) {
            std::cerr << "[PDO] Failed to send frame: " << strerror(errno) << std::endl;
            return false;
        }

        return true;
    }

// =============================================================================
// TPDO Reception (Feedback from Motors)
// =============================================================================

    void PDOManager::register_tpdo1_callback(uint8_t node_id, TPDOCallback callback) {
        uint32_t cob_id = tpdo1_cob_id(node_id);

        std::lock_guard<std::mutex> lock(callback_mutex_);
        tpdo_callbacks_[cob_id] = std::move(callback);

        std::cout << "[PDO] Registered TPDO1 callback for node "
                  << static_cast<int>(node_id)
                  << " (COB-ID: 0x" << std::hex << cob_id << std::dec << ")" << std::endl;
    }

    void PDOManager::register_tpdo2_callback(uint8_t node_id, TPDOCallback callback) {
        uint32_t cob_id = tpdo2_cob_id(node_id);

        std::lock_guard<std::mutex> lock(callback_mutex_);
        tpdo_callbacks_[cob_id] = std::move(callback);

        std::cout << "[PDO] Registered TPDO2 callback for node "
                  << static_cast<int>(node_id)
                  << " (COB-ID: 0x" << std::hex << cob_id << std::dec << ")" << std::endl;
    }

    void PDOManager::unregister_callbacks(uint8_t node_id) {
        std::lock_guard<std::mutex> lock(callback_mutex_);

        tpdo_callbacks_.erase(tpdo1_cob_id(node_id));
        tpdo_callbacks_.erase(tpdo2_cob_id(node_id));

        std::cout << "[PDO] Unregistered callbacks for node "
                  << static_cast<int>(node_id) << std::endl;
    }

// =============================================================================
// Receive Loop
// =============================================================================

    void PDOManager::receive_loop() {
        std::cout << "[PDO] Receive thread started" << std::endl;

        struct can_frame frame;

        while (running_.load()) {
            // Receive CAN frame (blocking with timeout)
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(socket_fd_, &readfds);

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000; // 100ms timeout

            int ret = select(socket_fd_ + 1, &readfds, nullptr, nullptr, &timeout);

            if (ret < 0) {
                if (errno != EINTR) {
                    std::cerr << "[PDO] select() error: " << strerror(errno) << std::endl;
                }
                continue;
            }

            if (ret == 0) {
                // Timeout - continue loop
                continue;
            }

            // Read frame
            ssize_t nbytes = recv(socket_fd_, &frame, sizeof(frame), 0);

            if (nbytes < 0) {
                std::cerr << "[PDO] recv() error: " << strerror(errno) << std::endl;
                continue;
            }

            if (nbytes != sizeof(frame)) {
                std::cerr << "[PDO] Incomplete frame received" << std::endl;
                continue;
            }

            // Dispatch to appropriate callback
            dispatch_tpdo(frame);
        }

        std::cout << "[PDO] Receive thread stopped" << std::endl;
    }

    void PDOManager::dispatch_tpdo(const can_frame& frame) {
        uint32_t cob_id = frame.can_id;

        // Look up callback
        std::lock_guard<std::mutex> lock(callback_mutex_);
        auto it = tpdo_callbacks_.find(cob_id);

        if (it != tpdo_callbacks_.end()) {
            // Extract node ID for statistics
            uint8_t node_id = extract_node_id(cob_id);

            if (node_id > 0) {
                auto& stats = stats_[node_id];
                auto now = std::chrono::steady_clock::now();

                // Determine which TPDO type and update lock-free atomic counters
                if (cob_id == tpdo1_cob_id(node_id)) {
                    stats.tpdo1_received.fetch_add(1, std::memory_order_relaxed);
                    stats.last_tpdo1_time = now;
                } else if (cob_id == tpdo2_cob_id(node_id)) {
                    stats.tpdo2_received.fetch_add(1, std::memory_order_relaxed);
                    stats.last_tpdo2_time = now;
                }
            }

            // Call registered callback (outside stats lock to avoid deadlock)
            try {
                it->second(frame);
            } catch (const std::exception& e) {
                std::cerr << "[PDO] Callback exception: " << e.what() << std::endl;
                if (node_id > 0) {
                    stats_[node_id].errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }
    }

// =============================================================================
// Statistics & Diagnostics
// =============================================================================

    PDOManager::StatisticsSnapshot PDOManager::get_statistics(uint8_t node_id) const {
        std::lock_guard<std::mutex> lock(stats_mutex_);

        auto it = stats_.find(node_id);
        if (it != stats_.end()) {
            const auto& stats = it->second;

            // Create non-atomic snapshot using memory_order_relaxed for performance
            StatisticsSnapshot snapshot;
            snapshot.tpdo1_received = stats.tpdo1_received.load(std::memory_order_relaxed);
            snapshot.tpdo2_received = stats.tpdo2_received.load(std::memory_order_relaxed);
            snapshot.rpdo1_sent = stats.rpdo1_sent.load(std::memory_order_relaxed);
            snapshot.rpdo2_sent = stats.rpdo2_sent.load(std::memory_order_relaxed);
            snapshot.errors = stats.errors.load(std::memory_order_relaxed);
            snapshot.avg_latency_us = stats.get_avg_latency_us();
            snapshot.last_tpdo1_time = stats.last_tpdo1_time;
            snapshot.last_tpdo2_time = stats.last_tpdo2_time;

            return snapshot;
        }

        // Return empty stats if not found
        return StatisticsSnapshot{};
    }

    void PDOManager::reset_statistics(uint8_t node_id) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        auto it = stats_.find(node_id);
        if (it != stats_.end()) {
            it->second.reset();
        }
    }

} // namespace canopen

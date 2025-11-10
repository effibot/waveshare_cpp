/**
 * @file sdo_client.hpp
 * @author effibot (andrea.efficace1@gmail.com)
 * @brief CANopen SDO Client using raw SocketCAN
 * @version 0.1
 * @date 2025-11-06
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include "canopen/object_dictionary.hpp"
#include "io/can_socket.hpp"
#include <linux/can.h>
#include <linux/can/raw.h>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <memory>

namespace canopen {

/**
 * @brief SDO Client using ICANSocket abstraction
 *
 * This client sends/receives CAN frames through the ICANSocket interface,
 * enabling dependency injection and mock-based testing.
 */
    class SDOClient {
        public:
            /**
             * @brief Construct SDO client with socket dependency injection
             * @param socket Shared pointer to CAN socket implementation
             * @param dictionary Reference to object dictionary
             * @param node_id CANopen node ID of the target device
             */
            explicit SDOClient(
                std::shared_ptr<waveshare::ICANSocket> socket,
                const ObjectDictionary& dictionary,
                uint8_t node_id
            );

            ~SDOClient();

            // Prevent copying (socket is a system resource)
            SDOClient(const SDOClient&) = delete;
            SDOClient& operator=(const SDOClient&) = delete;

            /**
             * @brief Write object via SDO (blocking with timeout)
             * @param object_name Name from object dictionary
             * @param data Raw bytes to write
             * @param timeout Maximum wait time for response
             * @return true if write successful
             */
            bool write_object(
                const std::string& object_name,
                const std::vector<uint8_t>& data,
                std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)
            );

            /**
             * @brief Read object via SDO (blocking with timeout)
             * @param object_name Name from object dictionary
             * @param timeout Maximum wait time for response
             * @return Raw bytes read from device
             */
            std::vector<uint8_t> read_object(
                const std::string& object_name,
                std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)
            );

            /**
             * @brief Type-safe write wrapper
             */
            template<typename T>
            bool write(const std::string& object_name, T value) {
                std::vector<uint8_t> data = dictionary_.to_raw(value);
                return write_object(object_name, data);
            }

            /**
             * @brief Type-safe read wrapper
             */
            template<typename T>
            T read(const std::string& object_name) {
                auto raw_data = read_object(object_name);
                return dictionary_.from_raw<T>(raw_data);
            }

            /**
             * @brief Check if socket is open and operational
             */
            bool is_open() const { return socket_ && socket_->is_open(); }

        private:
            std::shared_ptr<waveshare::ICANSocket> socket_;
            const ObjectDictionary& dictionary_;
            uint8_t node_id_;

            // SDO protocol helpers (CiA 301)
            can_frame create_sdo_write_expedited(uint16_t index, uint8_t subindex,
                const std::vector<uint8_t>& data);
            can_frame create_sdo_read_request(uint16_t index, uint8_t subindex);

            // Frame send/receive with timeout
            bool send_frame(const can_frame& frame);
            bool receive_frame(can_frame& frame, std::chrono::milliseconds timeout);

            // COB-ID calculations
            uint32_t sdo_tx_cob_id() const { return 0x600 + node_id_; } // Client → Server
            uint32_t sdo_rx_cob_id() const { return 0x580 + node_id_; } // Server → Client

            // SDO response validation
            bool validate_sdo_response(const can_frame& frame, uint16_t expected_index,
                uint8_t expected_subindex);
    };

} // namespace canopen
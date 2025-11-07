/**
 * @file pdo_constants.hpp
 * @brief CANopen PDO (Process Data Object) Constants and Definitions
 * @author effibot (andrea.efficace1@gmail.com)
 * @date 2025-11-07
 *
 * This header defines all constants for CANopen PDO communication.
 * Based on CiA 301 (CANopen Application Layer and Communication Profile).
 *
 * PDO (Process Data Object):
 * - Real-time data exchange mechanism in CANopen
 * - No protocol overhead (direct CAN frame mapping)
 * - Two types: RPDO (receive) and TPDO (transmit) from motor perspective
 *
 * @see CiA 301 v4.2.0 Section 7.2.2 (PDO Communication)
 */

#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>

namespace canopen {
    namespace pdo {

// =============================================================================
// COB-ID Base Addresses (CiA 301 Standard)
// =============================================================================

/**
 * @brief Standard CANopen PDO COB-ID base addresses
 *
 * CANopen uses predefined COB-ID ranges for PDO communication:
 * - COB-ID = BASE + node_id (where node_id is 1-127)
 *
 * Example for node_id = 1:
 * - RPDO1: 0x200 + 1 = 0x201
 * - RPDO2: 0x300 + 1 = 0x301
 * - TPDO1: 0x180 + 1 = 0x181
 * - TPDO2: 0x280 + 1 = 0x281
 */
        namespace cob_id {
            /**
             * @brief RPDO1 base COB-ID (Receive PDO 1 - commands FROM host TO motor)
             *
             * Typical mapping for CIA402 drives:
             * - Byte 0-1: Controlword (0x6040)
             * - Byte 2-5: Target Position (0x607A) or Target Velocity (0x60FF)
             */
            constexpr uint32_t RPDO1_BASE = 0x200;

            /**
             * @brief RPDO2 base COB-ID (Receive PDO 2 - additional commands)
             *
             * Typical mapping for CIA402 drives:
             * - Byte 0-3: Profile Velocity (0x6081)
             * - Byte 4-7: Profile Acceleration (0x6083)
             */
            constexpr uint32_t RPDO2_BASE = 0x300;

            /**
             * @brief RPDO3 base COB-ID (Receive PDO 3)
             */
            constexpr uint32_t RPDO3_BASE = 0x400;

            /**
             * @brief RPDO4 base COB-ID (Receive PDO 4)
             */
            constexpr uint32_t RPDO4_BASE = 0x500;

            /**
             * @brief TPDO1 base COB-ID (Transmit PDO 1 - feedback FROM motor TO host)
             *
             * Typical mapping for CIA402 drives:
             * - Byte 0-1: Statusword (0x6041)
             * - Byte 2-5: Position Actual Value (0x6064)
             */
            constexpr uint32_t TPDO1_BASE = 0x180;

            /**
             * @brief TPDO2 base COB-ID (Transmit PDO 2 - additional feedback)
             *
             * Typical mapping for CIA402 drives:
             * - Byte 0-3: Velocity Actual Value (0x606C)
             * - Byte 4-5: Current Actual Value (0x6078)
             */
            constexpr uint32_t TPDO2_BASE = 0x280;

            /**
             * @brief TPDO3 base COB-ID (Transmit PDO 3)
             */
            constexpr uint32_t TPDO3_BASE = 0x380;

            /**
             * @brief TPDO4 base COB-ID (Transmit PDO 4)
             */
            constexpr uint32_t TPDO4_BASE = 0x480;

            /**
             * @brief Maximum valid node ID in CANopen
             */
            constexpr uint8_t MAX_NODE_ID = 127;

            /**
             * @brief Minimum valid node ID in CANopen
             */
            constexpr uint8_t MIN_NODE_ID = 1;
        } // namespace cob_id

// =============================================================================
// PDO Types
// =============================================================================

/**
 * @brief PDO type enumeration
 */
        enum class PDOType : uint8_t {
            RPDO1 = 1, ///< Receive PDO 1 (host → motor)
            RPDO2 = 2, ///< Receive PDO 2 (host → motor)
            RPDO3 = 3, ///< Receive PDO 3 (host → motor)
            RPDO4 = 4, ///< Receive PDO 4 (host → motor)
            TPDO1 = 5, ///< Transmit PDO 1 (motor → host)
            TPDO2 = 6, ///< Transmit PDO 2 (motor → host)
            TPDO3 = 7, ///< Transmit PDO 3 (motor → host)
            TPDO4 = 8, ///< Transmit PDO 4 (motor → host)
        };

/**
 * @brief PDO transmission types (CiA 301)
 */
        enum class TransmissionType : uint8_t {
            SYNCHRONOUS_ACYCLIC = 0, ///< Triggered by SYNC, no automatic sending
            SYNCHRONOUS_CYCLIC_1 = 1, ///< Every 1st SYNC
            SYNCHRONOUS_CYCLIC_2 = 2, ///< Every 2nd SYNC
            SYNCHRONOUS_CYCLIC_240 = 240, ///< Every 240th SYNC
            RTR_ONLY_SYNC = 252,    ///< Remote transmission request with SYNC
            RTR_ONLY = 253,         ///< Remote transmission request
            EVENT_DRIVEN = 254,     ///< Asynchronous, manufacturer-specific
            EVENT_DRIVEN_RESERVED = 255, ///< Reserved
        };

// =============================================================================
// PDO Limits
// =============================================================================

        namespace limits {
            /**
             * @brief Maximum PDO data length in bytes (CAN standard frame limit)
             */
            constexpr uint8_t MAX_PDO_DATA_LENGTH = 8;

            /**
             * @brief Maximum number of PDOs per direction (RPDO or TPDO)
             */
            constexpr uint8_t MAX_PDOS_PER_DIRECTION = 4;

            /**
             * @brief Recommended PDO cycle time for real-time control (milliseconds)
             */
            constexpr uint32_t RECOMMENDED_CYCLE_MS = 10; // 100 Hz

            /**
             * @brief Minimum safe PDO cycle time (milliseconds)
             */
            constexpr uint32_t MIN_CYCLE_MS = 1; // 1000 Hz (theoretical CAN limit)
        } // namespace limits

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * @brief Calculate COB-ID for given PDO type and node ID
 * @param type PDO type (RPDO1, RPDO2, TPDO1, TPDO2, etc.)
 * @param node_id CANopen node ID (1-127)
 * @return Calculated COB-ID
 * @throws std::invalid_argument if node_id is out of range
 */
        inline uint32_t calculate_cob_id(PDOType type, uint8_t node_id) {
            if (node_id < cob_id::MIN_NODE_ID || node_id > cob_id::MAX_NODE_ID) {
                throw std::invalid_argument("Node ID must be between 1 and 127");
            }

            switch (type) {
            case PDOType::RPDO1: return cob_id::RPDO1_BASE + node_id;
            case PDOType::RPDO2: return cob_id::RPDO2_BASE + node_id;
            case PDOType::RPDO3: return cob_id::RPDO3_BASE + node_id;
            case PDOType::RPDO4: return cob_id::RPDO4_BASE + node_id;
            case PDOType::TPDO1: return cob_id::TPDO1_BASE + node_id;
            case PDOType::TPDO2: return cob_id::TPDO2_BASE + node_id;
            case PDOType::TPDO3: return cob_id::TPDO3_BASE + node_id;
            case PDOType::TPDO4: return cob_id::TPDO4_BASE + node_id;
            default:
                throw std::invalid_argument("Invalid PDO type");
            }
        }

/**
 * @brief Extract node ID from COB-ID
 * @param cob_id COB-ID from CAN frame
 * @return Node ID (1-127), or 0 if not a valid PDO COB-ID
 */
        inline uint8_t extract_node_id(uint32_t cob_id) {
            // TPDO1: 0x180 + node_id
            if (cob_id >= cob_id::TPDO1_BASE && cob_id < cob_id::RPDO1_BASE) {
                return static_cast<uint8_t>(cob_id - cob_id::TPDO1_BASE);
            }

            // RPDO1: 0x200 + node_id
            if (cob_id >= cob_id::RPDO1_BASE && cob_id < cob_id::TPDO2_BASE) {
                return static_cast<uint8_t>(cob_id - cob_id::RPDO1_BASE);
            }

            // TPDO2: 0x280 + node_id
            if (cob_id >= cob_id::TPDO2_BASE && cob_id < cob_id::RPDO2_BASE) {
                return static_cast<uint8_t>(cob_id - cob_id::TPDO2_BASE);
            }

            // RPDO2: 0x300 + node_id
            if (cob_id >= cob_id::RPDO2_BASE && cob_id < cob_id::TPDO3_BASE) {
                return static_cast<uint8_t>(cob_id - cob_id::RPDO2_BASE);
            }

            // TPDO3: 0x380 + node_id
            if (cob_id >= cob_id::TPDO3_BASE && cob_id < cob_id::RPDO3_BASE) {
                return static_cast<uint8_t>(cob_id - cob_id::TPDO3_BASE);
            }

            // RPDO3: 0x400 + node_id
            if (cob_id >= cob_id::RPDO3_BASE && cob_id < cob_id::TPDO4_BASE) {
                return static_cast<uint8_t>(cob_id - cob_id::RPDO3_BASE);
            }

            // TPDO4: 0x480 + node_id
            if (cob_id >= cob_id::TPDO4_BASE && cob_id < cob_id::RPDO4_BASE) {
                return static_cast<uint8_t>(cob_id - cob_id::TPDO4_BASE);
            }

            // RPDO4: 0x500 + node_id
            if (cob_id >= cob_id::RPDO4_BASE && cob_id < 0x580) {
                return static_cast<uint8_t>(cob_id - cob_id::RPDO4_BASE);
            }

            return 0; // Invalid COB-ID
        }

/**
 * @brief Determine PDO type from COB-ID
 * @param cob_id COB-ID from CAN frame
 * @return PDO type, or nullptr if not a valid PDO COB-ID
 */
        inline PDOType* get_pdo_type(uint32_t cob_id) {
            static PDOType type;

            if (cob_id >= cob_id::TPDO1_BASE && cob_id < cob_id::RPDO1_BASE) {
                type = PDOType::TPDO1;
                return &type;
            }
            if (cob_id >= cob_id::RPDO1_BASE && cob_id < cob_id::TPDO2_BASE) {
                type = PDOType::RPDO1;
                return &type;
            }
            if (cob_id >= cob_id::TPDO2_BASE && cob_id < cob_id::RPDO2_BASE) {
                type = PDOType::TPDO2;
                return &type;
            }
            if (cob_id >= cob_id::RPDO2_BASE && cob_id < cob_id::TPDO3_BASE) {
                type = PDOType::RPDO2;
                return &type;
            }
            if (cob_id >= cob_id::TPDO3_BASE && cob_id < cob_id::RPDO3_BASE) {
                type = PDOType::TPDO3;
                return &type;
            }
            if (cob_id >= cob_id::RPDO3_BASE && cob_id < cob_id::TPDO4_BASE) {
                type = PDOType::RPDO3;
                return &type;
            }
            if (cob_id >= cob_id::TPDO4_BASE && cob_id < cob_id::RPDO4_BASE) {
                type = PDOType::TPDO4;
                return &type;
            }
            if (cob_id >= cob_id::RPDO4_BASE && cob_id < 0x580) {
                type = PDOType::RPDO4;
                return &type;
            }

            return nullptr; // Invalid COB-ID
        }

/**
 * @brief Convert PDO type to string
 * @param type PDO type
 * @return Human-readable string
 */
        inline std::string pdo_type_to_string(PDOType type) {
            switch (type) {
            case PDOType::RPDO1: return "RPDO1";
            case PDOType::RPDO2: return "RPDO2";
            case PDOType::RPDO3: return "RPDO3";
            case PDOType::RPDO4: return "RPDO4";
            case PDOType::TPDO1: return "TPDO1";
            case PDOType::TPDO2: return "TPDO2";
            case PDOType::TPDO3: return "TPDO3";
            case PDOType::TPDO4: return "TPDO4";
            default: return "UNKNOWN";
            }
        }

    } // namespace pdo
} // namespace canopen

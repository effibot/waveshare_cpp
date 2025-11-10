/**
 * @file cia402_registers.hpp
 * @brief Type-safe CIA402 register definitions using enum classes
 * @author effibot (andrea.efficace1@gmail.com)
 * @date 2025-11-10
 *
 * This header provides type-safe enum classes for all CIA402 register indices.
 * Replaces scattered constexpr values with grouped, autocomplete-friendly enums.
 *
 * Benefits:
 * - Type safety: Cannot accidentally use wrong register index
 * - IDE autocomplete: Easy discovery of available registers
 * - Grouped by purpose: Communication, control, position, velocity, etc.
 * - Self-documenting: Enum names clearly indicate register purpose
 *
 * Usage:
 * @code
 * using Reg = canopen::cia402::CIA402Register;
 * sdo_client.read(to_index(Reg::STATUSWORD), 0);  // Type-safe!
 * @endcode
 *
 * References:
 * - CiA 301: CANopen Application Layer and Communication Profile
 * - CiA 402: CANopen Device Profile for Drives and Motion Control
 *
 * @copyright Copyright (c) 2025
 */

#pragma once

#include <cstdint>

namespace canopen {
    namespace cia402 {

/**
 * @brief CIA402 Object Dictionary Register Indices
 *
 * Complete enumeration of all standard CIA402 object dictionary indices.
 * Organized by functional groups for easy navigation.
 *
 * Use to_index() to convert to uint16_t when needed for SDO operations.
 */
        enum class CIA402Register : uint16_t {
            // =========================================================================
            // Communication Profile Objects (0x1000 - 0x1FFF) - CiA 301
            // =========================================================================

            /**
             * @brief Device Type (0x1000)
             *
             * Identifies the device type according to device profile.
             * For CIA402 drives: typically 0x00020192
             */
            DEVICE_TYPE = 0x1000,

            /**
             * @brief Error Register (0x1001)
             *
             * 8-bit register indicating error conditions:
             * - Bit 0: Generic error
             * - Bit 1: Current error
             * - Bit 2: Voltage error
             * - Bit 3: Temperature error
             * - Bit 4: Communication error
             * - Bit 5: Device profile specific
             * - Bit 7: Manufacturer specific
             */
            ERROR_REGISTER = 0x1001,

            /**
             * @brief Manufacturer Status Register (0x1002)
             *
             * Manufacturer-specific status information.
             * Content and format varies by manufacturer.
             */
            MANUFACTURER_STATUS = 0x1002,

            /**
             * @brief Predefined Error Field (0x1003)
             *
             * Array of error codes from error history.
             * Subindex 0: Number of errors
             * Subindex 1-n: Error codes
             */
            PREDEFINED_ERROR_FIELD = 0x1003,

            /**
             * @brief Identity Object (0x1018)
             *
             * Device identification information:
             * - Subindex 0: Number of entries (4)
             * - Subindex 1: Vendor ID
             * - Subindex 2: Product code
             * - Subindex 3: Revision number
             * - Subindex 4: Serial number
             */
            IDENTITY_OBJECT = 0x1018,

            // =========================================================================
            // Device Control & Status (0x6000 - 0x6FFF) - CiA 402
            // =========================================================================

            /**
             * @brief Controlword (0x6040)
             *
             * 16-bit command word for state machine control and operation commands.
             * See controlword bit definitions in cia402_constants.hpp
             */
            CONTROLWORD = 0x6040,

            /**
             * @brief Statusword (0x6041)
             *
             * 16-bit status word indicating current device state and conditions.
             * See statusword bit definitions in cia402_constants.hpp
             */
            STATUSWORD = 0x6041,

            /**
             * @brief Velocity Demand Value (0x6043)
             *
             * Velocity demand value from trajectory generator (int16)
             */
            VELOCITY_DEMAND = 0x6043,

            // =========================================================================
            // Mode Control (0x6060 - 0x6061)
            // =========================================================================

            /**
             * @brief Modes of Operation (0x6060)
             *
             * Selects the operation mode (int8):
             * - 0: No mode
             * - 1: Profile Position Mode
             * - 3: Profile Velocity Mode
             * - 4: Torque Profile Mode
             * - 6: Homing Mode
             * - 8: Cyclic Sync Position
             * - 9: Cyclic Sync Velocity
             * - 10: Cyclic Sync Torque
             */
            MODES_OF_OPERATION = 0x6060,

            /**
             * @brief Modes of Operation Display (0x6061)
             *
             * Displays the currently active operation mode (int8)
             * Same values as MODES_OF_OPERATION
             */
            MODES_OF_OPERATION_DISPLAY = 0x6061,

            // =========================================================================
            // Position Control (0x6062 - 0x607C)
            // =========================================================================

            /**
             * @brief Position Demand Value (0x6062)
             *
             * Position demand value from trajectory generator (int32)
             */
            POSITION_DEMAND = 0x6062,

            /**
             * @brief Position Actual Value (0x6064)
             *
             * Current motor position in encoder counts (int32)
             */
            POSITION_ACTUAL = 0x6064,

            /**
             * @brief Target Position (0x607A)
             *
             * Target position for Profile Position Mode (int32)
             */
            TARGET_POSITION = 0x607A,

            /**
             * @brief Home Offset (0x607C)
             *
             * Offset applied after homing operation (int32)
             */
            HOME_OFFSET = 0x607C,

            // =========================================================================
            // Velocity Control (0x606C - 0x6081)
            // =========================================================================

            /**
             * @brief Velocity Actual Value (0x606C)
             *
             * Current motor velocity (int32, typically in RPM or counts/s)
             */
            VELOCITY_ACTUAL = 0x606C,

            /**
             * @brief Target Velocity (0x60FF)
             *
             * Target velocity for velocity mode (int32)
             */
            TARGET_VELOCITY = 0x60FF,

            /**
             * @brief Max Motor Speed (0x6080)
             *
             * Maximum allowed motor speed (uint32)
             */
            MAX_MOTOR_SPEED = 0x6080,

            /**
             * @brief Profile Velocity (0x6081)
             *
             * Velocity for profile generation in position mode (uint32)
             */
            PROFILE_VELOCITY = 0x6081,

            // =========================================================================
            // Torque/Current Control (0x6071 - 0x6078)
            // =========================================================================

            /**
             * @brief Target Torque (0x6071)
             *
             * Target torque value for torque mode (int16)
             */
            TARGET_TORQUE = 0x6071,

            /**
             * @brief Max Current (0x6073)
             *
             * Maximum allowed motor current (uint16)
             */
            MAX_CURRENT = 0x6073,

            /**
             * @brief Motor Rated Current (0x6075)
             *
             * Nominal current of the motor (uint32)
             */
            MOTOR_RATED_CURRENT = 0x6075,

            /**
             * @brief Motor Rated Torque (0x6076)
             *
             * Nominal torque of the motor (uint32)
             */
            MOTOR_RATED_TORQUE = 0x6076,

            /**
             * @brief Torque Actual Value (0x6077)
             *
             * Current torque value (int16)
             */
            TORQUE_ACTUAL = 0x6077,

            /**
             * @brief Current Actual Value (0x6078)
             *
             * Current motor current in milliamps (int16)
             * Commonly used in TPDO2 for real-time feedback
             */
            CURRENT_ACTUAL = 0x6078,

            // =========================================================================
            // Profile Parameters (0x6083 - 0x6085)
            // =========================================================================

            /**
             * @brief Profile Acceleration (0x6083)
             *
             * Acceleration for profile generation (uint32)
             */
            PROFILE_ACCELERATION = 0x6083,

            /**
             * @brief Profile Deceleration (0x6084)
             *
             * Deceleration for profile generation (uint32)
             */
            PROFILE_DECELERATION = 0x6084,

            /**
             * @brief Quick Stop Deceleration (0x6085)
             *
             * Deceleration for quick stop function (uint32)
             */
            QUICK_STOP_DECELERATION = 0x6085,

            // =========================================================================
            // Digital I/O (0x60FD - 0x60FE)
            // =========================================================================

            /**
             * @brief Digital Inputs (0x60FD)
             *
             * State of digital input pins (uint32)
             */
            DIGITAL_INPUTS = 0x60FD,

            /**
             * @brief Digital Outputs (0x60FE)
             *
             * Control digital output pins (uint32)
             * Subindex 1: Physical outputs
             * Subindex 2: Bit mask
             */
            DIGITAL_OUTPUTS = 0x60FE,

            // =========================================================================
            // Manufacturer Specific (0x2000 - 0x5FFF)
            // =========================================================================

            /**
             * @brief Temperature (0x2205)
             *
             * Device/motor temperature (manufacturer specific)
             * Format varies by manufacturer
             */
            TEMPERATURE = 0x2205,
        };

/**
 * @brief Convert CIA402Register enum to uint16_t index
 *
 * Helper function to extract the underlying register index value.
 * Use this when calling SDO read/write functions.
 *
 * @param reg The register enum value
 * @return uint16_t The object dictionary index
 *
 * @example
 * @code
 * using Reg = canopen::cia402::CIA402Register;
 * uint16_t index = to_index(Reg::STATUSWORD);  // Returns 0x6041
 * sdo_client.read_u16(index, 0);
 * @endcode
 */
        constexpr uint16_t to_index(CIA402Register reg) {
            return static_cast<uint16_t>(reg);
        }

/**
 * @brief Convert uint16_t index to CIA402Register enum
 *
 * Helper function to convert index back to enum (use with caution).
 *
 * @param index The object dictionary index
 * @return CIA402Register The register enum value
 *
 * @warning Only use with known valid register indices
 */
        constexpr CIA402Register from_index(uint16_t index) {
            return static_cast<CIA402Register>(index);
        }

    } // namespace cia402
} // namespace canopen

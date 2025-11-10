/**
 * @file cia402_constants.hpp
 * @brief CIA402 CANopen Device Profile Constants and Definitions
 * @author effibot (andrea.efficace1@gmail.com)
 * @date 2025-11-10
 *
 * This header defines all constants for CIA402 (CANopen drives and motion control)
 * including state machine definitions and bit masks.
 *
 * Design Philosophy (2025-11-10):
 * - Type-safe enums ONLY (no raw constexpr values)
 * - Register indices: Use CIA402Register enum from cia402_registers.hpp
 * - COB-IDs: Use PDOCobIDBase enum from pdo_constants.hpp
 * - States: Use State enum (already defined here)
 * - Operation modes: Use OperationMode enum (already defined here)
 *
 * References:
 * - CiA 301: CANopen Application Layer and Communication Profile
 * - CiA 402: CANopen Device Profile for Drives and Motion Control
 */

#pragma once

#include <cstdint>
#include "cia402_registers.hpp"  // Type-safe register enums

namespace canopen {
    namespace cia402 {

// ==============================================================================
// CIA402 State Machine
// ==============================================================================

/**
 * @brief CIA402 device states
 *
 * The state machine defines the operational states of a drive device.
 * State transitions are controlled via the Controlword and monitored via Statusword.
 */
        enum class State : uint8_t {
            NOT_READY_TO_SWITCH_ON = 0, ///< Initial state, device is initializing
            SWITCH_ON_DISABLED = 1, ///< Device ready but voltage disabled
            READY_TO_SWITCH_ON = 2, ///< Device ready to switch on
            SWITCHED_ON = 3,       ///< Voltage enabled, but drive disabled
            OPERATION_ENABLED = 4, ///< Normal operation state
            QUICK_STOP_ACTIVE = 5, ///< Quick stop function active
            FAULT_REACTION_ACTIVE = 6, ///< Fault reaction in progress
            FAULT = 7,             ///< Fault state
            UNKNOWN = 0xFF         ///< State cannot be determined
        };

/**
 * @brief Operation modes defined by CIA402
 *
 * Each mode provides different control characteristics.
 * Value at object 0x6060 (Modes of Operation)
 */
        enum class OperationMode : int8_t {
            NO_MODE = 0,            ///< No mode selected
            PROFILE_POSITION = 1,   ///< Profile Position Mode (PP)
            VELOCITY = 2,           ///< Velocity Mode
            PROFILE_VELOCITY = 3,   ///< Profile Velocity Mode (PV)
            TORQUE_PROFILE = 4,     ///< Torque Profile Mode
            HOMING = 6,             ///< Homing Mode (HM)
            INTERPOLATED_POSITION = 7, ///< Interpolated Position Mode (IP)
            CYCLIC_SYNC_POSITION = 8, ///< Cyclic Synchronous Position Mode (CSP)
            CYCLIC_SYNC_VELOCITY = 9, ///< Cyclic Synchronous Velocity Mode (CSV)
            CYCLIC_SYNC_TORQUE = 10 ///< Cyclic Synchronous Torque Mode (CST)
        };

// ==============================================================================
// Statusword Bit Definitions (0x6041)
// ==============================================================================

/**
 * @brief Individual statusword bit masks
 *
 * Use for checking individual bits in statusword value.
 * Example: if (statusword & to_mask(StatuswordBit::FAULT))
 */
        enum class StatuswordBit : uint16_t {
            READY_TO_SWITCH_ON = 0x0001, ///< Bit 0: Ready to switch on
            SWITCHED_ON = 0x0002,        ///< Bit 1: Switched on
            OPERATION_ENABLED = 0x0004,  ///< Bit 2: Operation enabled
            FAULT = 0x0008,              ///< Bit 3: Fault
            VOLTAGE_ENABLED = 0x0010,    ///< Bit 4: Voltage enabled
            QUICK_STOP = 0x0020,         ///< Bit 5: Quick stop (0=active)
            SWITCH_ON_DISABLED = 0x0040, ///< Bit 6: Switch on disabled
            WARNING = 0x0080,            ///< Bit 7: Warning
            REMOTE = 0x0200,             ///< Bit 9: Remote (controlled via fieldbus)
            TARGET_REACHED = 0x0400,     ///< Bit 10: Target reached
            INTERNAL_LIMIT = 0x0800      ///< Bit 11: Internal limit active
        };

        /// Convert StatuswordBit to uint16_t for bitwise operations
        constexpr uint16_t to_mask(StatuswordBit bit) {
            return static_cast<uint16_t>(bit);
        }

/**
 * @brief Statusword state detection patterns
 *
 * Used internally by decode_statusword() to determine device state.
 * These patterns match specific bit combinations per CIA402 spec.
 */
        enum class StatuswordPattern : uint8_t {
            MASK = 0x6F,                     ///< Mask for state bits (bits 0-6, excluding bit 4)
            NOT_READY_TO_SWITCH_ON = 0x00,   ///< xxxx_x000
            SWITCH_ON_DISABLED = 0x40,       ///< xxxx_x100_0000
            READY_TO_SWITCH_ON = 0x21,       ///< xxxx_x010_0001
            SWITCHED_ON = 0x23,              ///< xxxx_x010_0011
            OPERATION_ENABLED = 0x27,        ///< xxxx_x010_0111
            QUICK_STOP_ACTIVE = 0x07,        ///< xxxx_x000_0111
            FAULT_REACTION_ACTIVE = 0x0F,    ///< xxxx_x000_1111
            FAULT = 0x08                     ///< xxxx_x000_1000
        };

        /// Convert StatuswordPattern to uint8_t
        constexpr uint8_t to_pattern(StatuswordPattern pattern) {
            return static_cast<uint8_t>(pattern);
        }

// ==============================================================================
// Controlword Bit Definitions (0x6040)
// ==============================================================================

/**
 * @brief Individual controlword bit masks
 *
 * Use for setting/clearing individual bits in controlword.
 * Example: controlword |= to_mask(ControlwordBit::SWITCH_ON)
 */
        enum class ControlwordBit : uint16_t {
            SWITCH_ON = 0x0001,         ///< Bit 0: Switch on
            ENABLE_VOLTAGE = 0x0002,    ///< Bit 1: Enable voltage
            QUICK_STOP = 0x0004,        ///< Bit 2: Quick stop (1=disabled, 0=active)
            ENABLE_OPERATION = 0x0008,  ///< Bit 3: Enable operation
            FAULT_RESET = 0x0080,       ///< Bit 7: Fault reset (rising edge)
            HALT = 0x0100               ///< Bit 8: Halt
        };

        /// Convert ControlwordBit to uint16_t for bitwise operations
        constexpr uint16_t to_mask(ControlwordBit bit) {
            return static_cast<uint16_t>(bit);
        }

/**
 * @brief Standard controlword command values
 *
 * Predefined command patterns for common state transitions.
 * Write these values directly to controlword register.
 * Example: sdo_write(CONTROLWORD, to_command(ControlwordCommand::SHUTDOWN))
 */
        enum class ControlwordCommand : uint16_t {
            SHUTDOWN = 0x0006,           ///< Shutdown: xxxx_xxxx_x110
            SWITCH_ON = 0x0007,          ///< Switch On: xxxx_xxxx_x111
            SWITCH_ON_ENABLE_OP = 0x000F,///< Switch On + Enable: xxxx_xxxx_1111
            DISABLE_VOLTAGE = 0x0000,    ///< Disable Voltage: xxxx_xxxx_xx0x
            QUICK_STOP = 0x0002,         ///< Quick Stop: xxxx_xxxx_x01x
            DISABLE_OPERATION = 0x0007,  ///< Disable Operation: xxxx_xxxx_x111
            ENABLE_OPERATION = 0x000F,   ///< Enable Operation: xxxx_xxxx_1111
            FAULT_RESET = 0x0080         ///< Fault Reset: 1xxx_xxxx_xxxx
        };

        /// Convert ControlwordCommand to uint16_t
        constexpr uint16_t to_command(ControlwordCommand cmd) {
            return static_cast<uint16_t>(cmd);
        }

// ==============================================================================
// Error Register Bit Definitions (0x1001)
// ==============================================================================

/**
 * @brief Error register bit masks
 *
 * Bits in the error register (object 0x1001) indicate different error types.
 * Example: if (error_reg & to_mask(ErrorRegisterBit::GENERIC))
 */
        enum class ErrorRegisterBit : uint8_t {
            GENERIC = 0x01,         ///< Bit 0: Generic error
            CURRENT = 0x02,         ///< Bit 1: Current error
            VOLTAGE = 0x04,         ///< Bit 2: Voltage error
            TEMPERATURE = 0x08,     ///< Bit 3: Temperature error
            COMMUNICATION = 0x10,   ///< Bit 4: Communication error
            DEVICE_PROFILE = 0x20,  ///< Bit 5: Device profile specific error
            MANUFACTURER = 0x80     ///< Bit 7: Manufacturer specific error
        };

        /// Convert ErrorRegisterBit to uint8_t for bitwise operations
        constexpr uint8_t to_mask(ErrorRegisterBit bit) {
            return static_cast<uint8_t>(bit);
        }

// ==============================================================================
// Helper Functions
// ==============================================================================

/**
 * @brief Convert CIA402 state enum to string
 * @param state The state to convert
 * @return Human-readable state name
 */
        inline const char* state_to_string(State state) {
            switch (state) {
            case State::NOT_READY_TO_SWITCH_ON: return "NOT_READY_TO_SWITCH_ON";
            case State::SWITCH_ON_DISABLED: return "SWITCH_ON_DISABLED";
            case State::READY_TO_SWITCH_ON: return "READY_TO_SWITCH_ON";
            case State::SWITCHED_ON: return "SWITCHED_ON";
            case State::OPERATION_ENABLED: return "OPERATION_ENABLED";
            case State::QUICK_STOP_ACTIVE: return "QUICK_STOP_ACTIVE";
            case State::FAULT_REACTION_ACTIVE: return "FAULT_REACTION_ACTIVE";
            case State::FAULT: return "FAULT";
            default: return "UNKNOWN";
            }
        }

/**
 * @brief Convert operation mode enum to string
 * @param mode The operation mode
 * @return Human-readable mode name
 */
        inline const char* mode_to_string(OperationMode mode) {
            switch (mode) {
            case OperationMode::NO_MODE: return "NO_MODE";
            case OperationMode::PROFILE_POSITION: return "PROFILE_POSITION";
            case OperationMode::VELOCITY: return "VELOCITY";
            case OperationMode::PROFILE_VELOCITY: return "PROFILE_VELOCITY";
            case OperationMode::TORQUE_PROFILE: return "TORQUE_PROFILE";
            case OperationMode::HOMING: return "HOMING";
            case OperationMode::INTERPOLATED_POSITION: return "INTERPOLATED_POSITION";
            case OperationMode::CYCLIC_SYNC_POSITION: return "CYCLIC_SYNC_POSITION";
            case OperationMode::CYCLIC_SYNC_VELOCITY: return "CYCLIC_SYNC_VELOCITY";
            case OperationMode::CYCLIC_SYNC_TORQUE: return "CYCLIC_SYNC_TORQUE";
            default: return "UNKNOWN_MODE";
            }
        }

/**
 * @brief Decode statusword into CIA402 state
 * @param statusword The statusword value from object 0x6041
 * @return The corresponding CIA402 state
 *
 * This function analyzes bits 0-6 of the statusword (excluding bit 4 - voltage enabled)
 * to determine the current state of the device according to CIA402 specification.
 */
        inline State decode_statusword(uint16_t statusword) {
            uint8_t state_bits = statusword & to_pattern(StatuswordPattern::MASK);

            switch (state_bits) {
            case to_pattern(
                StatuswordPattern::NOT_READY_TO_SWITCH_ON): return State::NOT_READY_TO_SWITCH_ON;
            case to_pattern(
                StatuswordPattern::SWITCH_ON_DISABLED): return State::SWITCH_ON_DISABLED;
            case to_pattern(
                StatuswordPattern::READY_TO_SWITCH_ON): return State::READY_TO_SWITCH_ON;
            case to_pattern(StatuswordPattern::SWITCHED_ON): return State::SWITCHED_ON;
            case to_pattern(StatuswordPattern::OPERATION_ENABLED): return State::OPERATION_ENABLED;
            case to_pattern(StatuswordPattern::QUICK_STOP_ACTIVE): return State::QUICK_STOP_ACTIVE;
            case to_pattern(
                StatuswordPattern::FAULT_REACTION_ACTIVE): return State::FAULT_REACTION_ACTIVE;
            case to_pattern(StatuswordPattern::FAULT): return State::FAULT;
            default: return State::UNKNOWN;
            }
        }

/**
 * @brief Check if device has a fault
 * @param statusword The statusword value
 * @return true if fault bit is set
 */
        inline bool has_fault(uint16_t statusword) {
            return (statusword & to_mask(StatuswordBit::FAULT)) != 0;
        }

/**
 * @brief Check if device has a warning
 * @param statusword The statusword value
 * @return true if warning bit is set
 */
        inline bool has_warning(uint16_t statusword) {
            return (statusword & to_mask(StatuswordBit::WARNING)) != 0;
        }

/**
 * @brief Check if target has been reached
 * @param statusword The statusword value
 * @return true if target reached bit is set
 */
        inline bool target_reached(uint16_t statusword) {
            return (statusword & to_mask(StatuswordBit::TARGET_REACHED)) != 0;
        }

/**
 * @brief Check if device is in operational state
 * @param statusword The statusword value
 * @return true if device is in OPERATION_ENABLED state
 */
        inline bool is_operational(uint16_t statusword) {
            return decode_statusword(statusword) == State::OPERATION_ENABLED;
        }

/**
 * @brief Check if voltage is enabled
 * @param statusword The statusword value
 * @return true if voltage enabled bit is set
 */
        inline bool voltage_enabled(uint16_t statusword) {
            return (statusword & to_mask(StatuswordBit::VOLTAGE_ENABLED)) != 0;
        }

    } // namespace cia402
} // namespace canopen

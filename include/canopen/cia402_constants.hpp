/**
 * @file cia402_constants.hpp
 * @brief CIA402 CANopen Device Profile Constants and Definitions
 * @author effibot (andrea.efficace1@gmail.com)
 * @date 2025-11-06
 *
 * This header defines all constants for CIA402 (CANopen drives and motion control)
 * including object dictionary indices, state machine definitions, and bit masks.
 *
 * References:
 * - CiA 301: CANopen Application Layer and Communication Profile
 * - CiA 402: CANopen Device Profile for Drives and Motion Control
 */

#pragma once

#include <cstdint>

namespace canopen {
    namespace cia402 {

// ==============================================================================
// CANopen Object Dictionary Indices (CiA 301 & CiA 402)
// ==============================================================================

// --- Communication Profile Objects (0x1000 - 0x1FFF) ---
        constexpr uint16_t DEVICE_TYPE = 0x1000;
        constexpr uint16_t ERROR_REGISTER = 0x1001;
        constexpr uint16_t PREDEFINED_ERROR_FIELD = 0x1003;
        constexpr uint16_t IDENTITY_OBJECT = 0x1018;

// --- Device Control & Status (0x6000 - 0x6FFF) ---
        constexpr uint16_t CONTROLWORD = 0x6040;
        constexpr uint16_t STATUSWORD = 0x6041;
        constexpr uint16_t VELOCITY_DEMAND = 0x6043;

// --- Mode Control ---
        constexpr uint16_t MODES_OF_OPERATION = 0x6060;
        constexpr uint16_t MODES_OF_OPERATION_DISPLAY = 0x6061;

// --- Position Control ---
        constexpr uint16_t POSITION_DEMAND = 0x6062;
        constexpr uint16_t POSITION_ACTUAL = 0x6064;
        constexpr uint16_t TARGET_POSITION = 0x607A;
        constexpr uint16_t HOME_OFFSET = 0x607C;

// --- Velocity Control ---
        constexpr uint16_t VELOCITY_ACTUAL = 0x606C;
        constexpr uint16_t TARGET_VELOCITY = 0x60FF;
        constexpr uint16_t MAX_MOTOR_SPEED = 0x6080;
        constexpr uint16_t PROFILE_VELOCITY = 0x6081;

// --- Torque Control ---
        constexpr uint16_t TARGET_TORQUE = 0x6071;
        constexpr uint16_t MAX_CURRENT = 0x6073;
        constexpr uint16_t MOTOR_RATED_CURRENT = 0x6075;
        constexpr uint16_t MOTOR_RATED_TORQUE = 0x6076;
        constexpr uint16_t TORQUE_ACTUAL = 0x6077;

// --- Profile Parameters ---
        constexpr uint16_t PROFILE_ACCELERATION = 0x6083;
        constexpr uint16_t PROFILE_DECELERATION = 0x6084;
        constexpr uint16_t QUICK_STOP_DECELERATION = 0x6085;

// --- Digital I/O ---
        constexpr uint16_t DIGITAL_INPUTS = 0x60FD;
        constexpr uint16_t DIGITAL_OUTPUTS = 0x60FE;

// --- Manufacturer Specific (0x2000 - 0x5FFF) ---
// Note: These are manufacturer-specific and may vary
        constexpr uint16_t TEMPERATURE = 0x2205;

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

// Individual bit masks
        constexpr uint16_t SW_READY_TO_SWITCH_ON = 0x0001; ///< Bit 0: Ready to switch on
        constexpr uint16_t SW_SWITCHED_ON = 0x0002;  ///< Bit 1: Switched on
        constexpr uint16_t SW_OPERATION_ENABLED = 0x0004; ///< Bit 2: Operation enabled
        constexpr uint16_t SW_FAULT = 0x0008;        ///< Bit 3: Fault
        constexpr uint16_t SW_VOLTAGE_ENABLED = 0x0010; ///< Bit 4: Voltage enabled
        constexpr uint16_t SW_QUICK_STOP = 0x0020;   ///< Bit 5: Quick stop (0=active)
        constexpr uint16_t SW_SWITCH_ON_DISABLED = 0x0040; ///< Bit 6: Switch on disabled
        constexpr uint16_t SW_WARNING = 0x0080;      ///< Bit 7: Warning
        constexpr uint16_t SW_REMOTE = 0x0200;       ///< Bit 9: Remote (controlled via fieldbus)
        constexpr uint16_t SW_TARGET_REACHED = 0x0400; ///< Bit 10: Target reached
        constexpr uint16_t SW_INTERNAL_LIMIT = 0x0800; ///< Bit 11: Internal limit active

// State determination patterns (mask bits 0-6, excluding bit 4)
        constexpr uint8_t SW_MASK = 0x6F;                          ///< Mask for state bits
        constexpr uint8_t SW_NOT_READY_TO_SWITCH_ON_PATTERN = 0x00; ///< xxxx_x000
        constexpr uint8_t SW_SWITCH_ON_DISABLED_PATTERN = 0x40;    ///< xxxx_x100_0000
        constexpr uint8_t SW_READY_TO_SWITCH_ON_PATTERN = 0x21;    ///< xxxx_x010_0001
        constexpr uint8_t SW_SWITCHED_ON_PATTERN = 0x23;           ///< xxxx_x010_0011
        constexpr uint8_t SW_OPERATION_ENABLED_PATTERN = 0x27;     ///< xxxx_x010_0111
        constexpr uint8_t SW_QUICK_STOP_ACTIVE_PATTERN = 0x07;     ///< xxxx_x000_0111
        constexpr uint8_t SW_FAULT_REACTION_ACTIVE_PATTERN = 0x0F; ///< xxxx_x000_1111
        constexpr uint8_t SW_FAULT_PATTERN = 0x08;                 ///< xxxx_x000_1000

// ==============================================================================
// Controlword Bit Definitions (0x6040)
// ==============================================================================

// Individual bit masks
        constexpr uint16_t CW_SWITCH_ON_BIT = 0x0001;  ///< Bit 0: Switch on
        constexpr uint16_t CW_ENABLE_VOLTAGE_BIT = 0x0002; ///< Bit 1: Enable voltage
        constexpr uint16_t CW_QUICK_STOP_BIT = 0x0004; ///< Bit 2: Quick stop (1=disabled, 0=active)
        constexpr uint16_t CW_ENABLE_OPERATION_BIT = 0x0008; ///< Bit 3: Enable operation
        constexpr uint16_t CW_FAULT_RESET_BIT = 0x0080; ///< Bit 7: Fault reset (rising edge)
        constexpr uint16_t CW_HALT_BIT = 0x0100;       ///< Bit 8: Halt

// Standard controlword commands
        constexpr uint16_t CW_SHUTDOWN = 0x0006;       ///< Shutdown: xxxx_xxxx_x110
        constexpr uint16_t CW_SWITCH_ON = 0x0007;      ///< Switch On: xxxx_xxxx_x111
        constexpr uint16_t CW_SWITCH_ON_ENABLE_OP = 0x000F; ///< Switch On + Enable: xxxx_xxxx_1111
        constexpr uint16_t CW_DISABLE_VOLTAGE = 0x0000; ///< Disable Voltage: xxxx_xxxx_xx0x
        constexpr uint16_t CW_QUICK_STOP = 0x0002;     ///< Quick Stop: xxxx_xxxx_x01x
        constexpr uint16_t CW_DISABLE_OPERATION = 0x0007; ///< Disable Operation: xxxx_xxxx_x111
        constexpr uint16_t CW_ENABLE_OPERATION = 0x000F; ///< Enable Operation: xxxx_xxxx_1111
        constexpr uint16_t CW_FAULT_RESET = 0x0080;    ///< Fault Reset: 1xxx_xxxx_xxxx

// ==============================================================================
// Error Register Bit Definitions (0x1001)
// ==============================================================================

        constexpr uint8_t ERR_GENERIC = 0x01;   ///< Bit 0: Generic error
        constexpr uint8_t ERR_CURRENT = 0x02;   ///< Bit 1: Current error
        constexpr uint8_t ERR_VOLTAGE = 0x04;   ///< Bit 2: Voltage error
        constexpr uint8_t ERR_TEMPERATURE = 0x08; ///< Bit 3: Temperature error
        constexpr uint8_t ERR_COMMUNICATION = 0x10; ///< Bit 4: Communication error
        constexpr uint8_t ERR_DEVICE_PROFILE = 0x20; ///< Bit 5: Device profile specific error
        constexpr uint8_t ERR_MANUFACTURER = 0x80; ///< Bit 7: Manufacturer specific error

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
            uint8_t state_bits = statusword & SW_MASK;

            switch (state_bits) {
            case SW_NOT_READY_TO_SWITCH_ON_PATTERN: return State::NOT_READY_TO_SWITCH_ON;
            case SW_SWITCH_ON_DISABLED_PATTERN: return State::SWITCH_ON_DISABLED;
            case SW_READY_TO_SWITCH_ON_PATTERN: return State::READY_TO_SWITCH_ON;
            case SW_SWITCHED_ON_PATTERN: return State::SWITCHED_ON;
            case SW_OPERATION_ENABLED_PATTERN: return State::OPERATION_ENABLED;
            case SW_QUICK_STOP_ACTIVE_PATTERN: return State::QUICK_STOP_ACTIVE;
            case SW_FAULT_REACTION_ACTIVE_PATTERN: return State::FAULT_REACTION_ACTIVE;
            case SW_FAULT_PATTERN: return State::FAULT;
            default: return State::UNKNOWN;
            }
        }

/**
 * @brief Check if device has a fault
 * @param statusword The statusword value
 * @return true if fault bit is set
 */
        inline bool has_fault(uint16_t statusword) {
            return (statusword & SW_FAULT) != 0;
        }

/**
 * @brief Check if device has a warning
 * @param statusword The statusword value
 * @return true if warning bit is set
 */
        inline bool has_warning(uint16_t statusword) {
            return (statusword & SW_WARNING) != 0;
        }

/**
 * @brief Check if target has been reached
 * @param statusword The statusword value
 * @return true if target reached bit is set
 */
        inline bool target_reached(uint16_t statusword) {
            return (statusword & SW_TARGET_REACHED) != 0;
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
            return (statusword & SW_VOLTAGE_ENABLED) != 0;
        }

    } // namespace cia402
} // namespace canopen

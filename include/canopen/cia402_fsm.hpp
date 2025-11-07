/**
 * @file cia402_fsm.hpp
 * @brief CIA402 Finite State Machine for motor control
 * @author effibot (andrea.efficace1@gmail.com)
 * @date 2025-11-06
 *
 * Implements the CIA402 state machine for controlling motor drivers.
 * Provides high-level methods for motor enable/disable and state transitions.
 */

#pragma once

#include "canopen/sdo_client.hpp"
#include "canopen/object_dictionary.hpp"
#include "canopen/cia402_constants.hpp"
#include <chrono>
#include <string>
#include <vector>

namespace canopen {

/**
 * @class CIA402FSM
 * @brief Finite State Machine for CIA402 motor control
 *
 * This class manages the state transitions of a CIA402-compliant motor driver.
 * It provides methods for:
 * - Motor enable/disable sequences
 * - Fault handling and reset
 * - State monitoring
 * - Automatic state transitions
 */
    class CIA402FSM {
        public:
            /**
             * @brief Construct CIA402 state machine
             * @param sdo_client Reference to SDO client for communication
             * @param dictionary Reference to object dictionary
             * @param state_timeout Default timeout for state transitions (default: 1000ms)
             */
            explicit CIA402FSM(
                SDOClient& sdo_client,
                const ObjectDictionary& dictionary,
                std::chrono::milliseconds state_timeout = std::chrono::milliseconds(1000)
            );

            // =========================================================================
            // High-Level Control Methods
            // =========================================================================

            /**
             * @brief Enable motor operation (complete startup sequence)
             *
             * Performs the complete state transition sequence:
             * Current State → SWITCH_ON_DISABLED → READY_TO_SWITCH_ON → SWITCHED_ON → OPERATION_ENABLED
             *
             * @return true if motor successfully enabled
             */
            bool enable_operation();

            /**
             * @brief Disable motor operation (graceful shutdown)
             *
             * Transitions from OPERATION_ENABLED to READY_TO_SWITCH_ON
             *
             * @return true if successfully disabled
             */
            bool disable_operation();

            /**
             * @brief Execute quick stop (emergency stop)
             *
             * Immediately stops motor motion according to quick stop deceleration profile
             *
             * @return true if quick stop executed
             */
            bool quick_stop();

            /**
             * @brief Reset fault condition
             *
             * Clears fault state and transitions to SWITCH_ON_DISABLED
             *
             * @return true if fault cleared successfully
             */
            bool reset_fault();

            /**
             * @brief Shutdown the motor (transition to READY_TO_SWITCH_ON)
             *
             * @return true if successfully transitioned
             */
            bool shutdown();

            /**
             * @brief Switch on the motor (transition to SWITCHED_ON)
             *
             * @return true if successfully transitioned
             */
            bool switch_on();

            // =========================================================================
            // State Query Methods
            // =========================================================================

            /**
             * @brief Get current motor state
             * @param force_update If true, reads statusword from device (default: false)
             * @return Current CIA402 state
             */
            cia402::State get_current_state(bool force_update = false);

            /**
             * @brief Get current motor state as string
             * @param force_update If true, reads statusword from device
             * @return Human-readable state name
             */
            std::string get_current_state_string(bool force_update = false);

            /**
             * @brief Check if motor is operational
             * @return true if in OPERATION_ENABLED state
             */
            bool is_operational();

            /**
             * @brief Check if motor has fault
             * @return true if fault bit is set in statusword
             */
            bool has_fault();

            /**
             * @brief Check if motor has warning
             * @return true if warning bit is set in statusword
             */
            bool has_warning();

            /**
             * @brief Check if target has been reached
             * @return true if target reached bit is set
             */
            bool target_reached();

            /**
             * @brief Check if voltage is enabled
             * @return true if voltage enabled bit is set
             */
            bool voltage_enabled();

            /**
             * @brief Get last read statusword value
             * @return Statusword value
             */
            uint16_t get_statusword() const { return last_statusword_; }

            // =========================================================================
            // Configuration Methods
            // =========================================================================

            /**
             * @brief Set state transition timeout
             * @param timeout Timeout duration
             */
            void set_state_timeout(std::chrono::milliseconds timeout) {
                state_timeout_ = timeout;
            }

            /**
             * @brief Get current state transition timeout
             * @return Timeout duration
             */
            std::chrono::milliseconds get_state_timeout() const {
                return state_timeout_;
            }

        private:
            // =========================================================================
            // Internal Helper Methods
            // =========================================================================

            /**
             * @brief Wait for device to reach expected state
             * @param expected_state Target state
             * @param timeout Maximum wait time
             * @return true if state reached within timeout
             */
            bool wait_for_state(cia402::State expected_state, std::chrono::milliseconds timeout);

            /**
             * @brief Send controlword command to device
             * @param command Controlword value
             * @return true if command sent successfully
             */
            bool send_controlword(uint16_t command);

            /**
             * @brief Read statusword from device
             * @return Statusword value
             */
            uint16_t read_statusword();

            /**
             * @brief Update internal state from statusword
             */
            void update_state();

            /**
             * @brief Validate if transition to target state is possible
             * @param target Target state
             * @return true if transition is valid from current state
             */
            bool can_transition_to(cia402::State target);

            /**
             * @brief Get sequence of states to transition from current to target
             * @param target Target state
             * @return Vector of intermediate states
             */
            std::vector<cia402::State> get_transition_path(cia402::State target);

            /**
             * @brief Execute single state transition
             * @param target Target state
             * @return true if transition successful
             */
            bool execute_transition(cia402::State target);

            // =========================================================================
            // Member Variables
            // =========================================================================

            SDOClient& sdo_client_;                 ///< SDO client for communication
            const ObjectDictionary& dictionary_;    ///< Object dictionary reference

            cia402::State current_state_;           ///< Cached current state
            uint16_t last_statusword_;              ///< Last read statusword value
            std::chrono::milliseconds state_timeout_; ///< Default state transition timeout

            bool state_cache_valid_;                ///< Flag indicating if cached state is valid
    };

} // namespace canopen

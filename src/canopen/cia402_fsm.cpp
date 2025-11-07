/**
 * @file cia402_fsm.cpp
 * @brief CIA402 Finite State Machine implementation
 * @author effibot (andrea.efficace1@gmail.com)
 * @date 2025-11-06
 */

#include "canopen/cia402_fsm.hpp"
#include <iostream>
#include <thread>

namespace canopen {

    CIA402FSM::CIA402FSM(
        SDOClient& sdo_client,
        const ObjectDictionary& dictionary,
        std::chrono::milliseconds state_timeout
    ) : sdo_client_(sdo_client),
        dictionary_(dictionary),
        current_state_(cia402::State::UNKNOWN),
        last_statusword_(0),
        state_timeout_(state_timeout),
        state_cache_valid_(false) {
        // Read initial state
        update_state();
    }

// =============================================================================
// High-Level Control Methods
// =============================================================================

    bool CIA402FSM::enable_operation() {
        std::cout << "[CIA402] Starting enable operation sequence..." << std::endl;

        // Read current state
        update_state();
        std::cout << "[CIA402] Current state: " << cia402::state_to_string(current_state_) <<
            std::endl;

        // If in fault, try to reset
        if (current_state_ == cia402::State::FAULT) {
            std::cout << "[CIA402] Device in FAULT, attempting reset..." << std::endl;
            if (!reset_fault()) {
                std::cerr << "[CIA402] Failed to reset fault" << std::endl;
                return false;
            }
        }

        // Already operational?
        if (current_state_ == cia402::State::OPERATION_ENABLED) {
            std::cout << "[CIA402] Already in OPERATION_ENABLED state" << std::endl;
            return true;
        }

        // Execute state transitions based on current state
        if (current_state_ == cia402::State::NOT_READY_TO_SWITCH_ON) {
            std::cout <<
                "[CIA402] Waiting for automatic transition from NOT_READY_TO_SWITCH_ON..." <<
                std::endl;
            if (!wait_for_state(cia402::State::SWITCH_ON_DISABLED,
                std::chrono::milliseconds(5000))) {
                std::cerr << "[CIA402] Timeout waiting for SWITCH_ON_DISABLED" << std::endl;
                return false;
            }
        }

        // Shutdown: Transition to READY_TO_SWITCH_ON
        if (current_state_ == cia402::State::SWITCH_ON_DISABLED) {
            std::cout << "[CIA402] Sending Shutdown command (0x06)..." << std::endl;
            if (!send_controlword(cia402::CW_SHUTDOWN)) {
                std::cerr << "[CIA402] Failed to send shutdown command" << std::endl;
                return false;
            }

            if (!wait_for_state(cia402::State::READY_TO_SWITCH_ON, state_timeout_)) {
                std::cerr << "[CIA402] Timeout waiting for READY_TO_SWITCH_ON" << std::endl;
                return false;
            }
            std::cout << "[CIA402] ✓ Reached READY_TO_SWITCH_ON" << std::endl;
        }

        // Switch On: Transition to SWITCHED_ON
        if (current_state_ == cia402::State::READY_TO_SWITCH_ON) {
            std::cout << "[CIA402] Sending Switch On command (0x07)..." << std::endl;
            if (!send_controlword(cia402::CW_SWITCH_ON)) {
                std::cerr << "[CIA402] Failed to send switch on command" << std::endl;
                return false;
            }

            if (!wait_for_state(cia402::State::SWITCHED_ON, state_timeout_)) {
                std::cerr << "[CIA402] Timeout waiting for SWITCHED_ON" << std::endl;
                return false;
            }
            std::cout << "[CIA402] ✓ Reached SWITCHED_ON" << std::endl;
        }

        // Enable Operation: Transition to OPERATION_ENABLED
        if (current_state_ == cia402::State::SWITCHED_ON) {
            std::cout << "[CIA402] Sending Enable Operation command (0x0F)..." << std::endl;
            if (!send_controlword(cia402::CW_ENABLE_OPERATION)) {
                std::cerr << "[CIA402] Failed to send enable operation command" << std::endl;
                return false;
            }

            if (!wait_for_state(cia402::State::OPERATION_ENABLED, state_timeout_)) {
                std::cerr << "[CIA402] Timeout waiting for OPERATION_ENABLED" << std::endl;
                return false;
            }
            std::cout << "[CIA402] ✓ Reached OPERATION_ENABLED" << std::endl;
        }

        std::cout << "[CIA402] ✓ Motor enabled successfully!" << std::endl;
        return true;
    }

    bool CIA402FSM::disable_operation() {
        std::cout << "[CIA402] Disabling operation..." << std::endl;

        update_state();

        if (current_state_ != cia402::State::OPERATION_ENABLED) {
            std::cout << "[CIA402] Not in OPERATION_ENABLED state, nothing to disable" << std::endl;
            return true;
        }

        // Send disable operation command (switch back to SWITCHED_ON)
        if (!send_controlword(cia402::CW_DISABLE_OPERATION)) {
            std::cerr << "[CIA402] Failed to send disable operation command" << std::endl;
            return false;
        }

        if (!wait_for_state(cia402::State::SWITCHED_ON, state_timeout_)) {
            std::cerr << "[CIA402] Timeout waiting for SWITCHED_ON" << std::endl;
            return false;
        }

        std::cout << "[CIA402] ✓ Operation disabled" << std::endl;
        return true;
    }

    bool CIA402FSM::quick_stop() {
        std::cout << "[CIA402] Executing quick stop..." << std::endl;

        if (!send_controlword(cia402::CW_QUICK_STOP)) {
            std::cerr << "[CIA402] Failed to send quick stop command" << std::endl;
            return false;
        }

        if (!wait_for_state(cia402::State::QUICK_STOP_ACTIVE, state_timeout_)) {
            std::cerr << "[CIA402] Timeout waiting for QUICK_STOP_ACTIVE" << std::endl;
            return false;
        }

        std::cout << "[CIA402] ✓ Quick stop executed" << std::endl;
        return true;
    }

    bool CIA402FSM::reset_fault() {
        std::cout << "[CIA402] Resetting fault..." << std::endl;

        update_state();

        if (current_state_ != cia402::State::FAULT) {
            std::cout << "[CIA402] Not in FAULT state, nothing to reset" << std::endl;
            return true;
        }

        // Send fault reset command (rising edge trigger on bit 7)
        // First send without bit, then with bit
        if (!send_controlword(0x0000)) {
            std::cerr << "[CIA402] Failed to clear controlword for fault reset" << std::endl;
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (!send_controlword(cia402::CW_FAULT_RESET)) {
            std::cerr << "[CIA402] Failed to send fault reset command" << std::endl;
            return false;
        }

        // Wait for transition to SWITCH_ON_DISABLED
        if (!wait_for_state(cia402::State::SWITCH_ON_DISABLED, state_timeout_)) {
            std::cerr << "[CIA402] Timeout waiting for SWITCH_ON_DISABLED after fault reset" <<
                std::endl;
            return false;
        }

        std::cout << "[CIA402] ✓ Fault reset successful" << std::endl;
        return true;
    }

    bool CIA402FSM::shutdown() {
        std::cout << "[CIA402] Executing shutdown..." << std::endl;

        if (!send_controlword(cia402::CW_SHUTDOWN)) {
            std::cerr << "[CIA402] Failed to send shutdown command" << std::endl;
            return false;
        }

        if (!wait_for_state(cia402::State::READY_TO_SWITCH_ON, state_timeout_)) {
            std::cerr << "[CIA402] Timeout waiting for READY_TO_SWITCH_ON" << std::endl;
            return false;
        }

        std::cout << "[CIA402] ✓ Shutdown complete" << std::endl;
        return true;
    }

    bool CIA402FSM::switch_on() {
        std::cout << "[CIA402] Switching on..." << std::endl;

        update_state();

        if (current_state_ != cia402::State::READY_TO_SWITCH_ON) {
            std::cerr << "[CIA402] Can only switch on from READY_TO_SWITCH_ON state" << std::endl;
            return false;
        }

        if (!send_controlword(cia402::CW_SWITCH_ON)) {
            std::cerr << "[CIA402] Failed to send switch on command" << std::endl;
            return false;
        }

        if (!wait_for_state(cia402::State::SWITCHED_ON, state_timeout_)) {
            std::cerr << "[CIA402] Timeout waiting for SWITCHED_ON" << std::endl;
            return false;
        }

        std::cout << "[CIA402] ✓ Switched on" << std::endl;
        return true;
    }

// =============================================================================
// State Query Methods
// =============================================================================

    cia402::State CIA402FSM::get_current_state(bool force_update) {
        if (force_update || !state_cache_valid_) {
            update_state();
        }
        return current_state_;
    }

    std::string CIA402FSM::get_current_state_string(bool force_update) {
        return cia402::state_to_string(get_current_state(force_update));
    }

    bool CIA402FSM::is_operational() {
        update_state();
        return current_state_ == cia402::State::OPERATION_ENABLED;
    }

    bool CIA402FSM::has_fault() {
        update_state();
        return cia402::has_fault(last_statusword_);
    }

    bool CIA402FSM::has_warning() {
        update_state();
        return cia402::has_warning(last_statusword_);
    }

    bool CIA402FSM::target_reached() {
        update_state();
        return cia402::target_reached(last_statusword_);
    }

    bool CIA402FSM::voltage_enabled() {
        update_state();
        return cia402::voltage_enabled(last_statusword_);
    }

// =============================================================================
// Internal Helper Methods
// =============================================================================

    bool CIA402FSM::wait_for_state(cia402::State expected_state,
        std::chrono::milliseconds timeout) {
        auto start_time = std::chrono::steady_clock::now();

        while (std::chrono::steady_clock::now() - start_time < timeout) {
            update_state();

            if (current_state_ == expected_state) {
                return true;
            }

            // Check for fault
            if (current_state_ == cia402::State::FAULT) {
                std::cerr << "[CIA402] Device entered FAULT state while waiting for "
                          << cia402::state_to_string(expected_state) << std::endl;
                return false;
            }

            // Small delay before retry
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        std::cerr << "[CIA402] Timeout waiting for state " <<
            cia402::state_to_string(expected_state)
                  << ", current state: " << cia402::state_to_string(current_state_) << std::endl;
        return false;
    }

    bool CIA402FSM::send_controlword(uint16_t command) {
        try {
            bool success = sdo_client_.write<uint16_t>("controlword", command);
            if (!success) {
                std::cerr << "[CIA402] SDO write failed for controlword" << std::endl;
                return false;
            }

            // Small delay to allow device to process command
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return true;

        } catch (const std::exception& e) {
            std::cerr << "[CIA402] Exception sending controlword: " << e.what() << std::endl;
            return false;
        }
    }

    uint16_t CIA402FSM::read_statusword() {
        try {
            return sdo_client_.read<uint16_t>("statusword");
        } catch (const std::exception& e) {
            std::cerr << "[CIA402] Exception reading statusword: " << e.what() << std::endl;
            return 0;
        }
    }

    void CIA402FSM::update_state() {
        last_statusword_ = read_statusword();
        current_state_ = cia402::decode_statusword(last_statusword_);
        state_cache_valid_ = true;
    }

    bool CIA402FSM::can_transition_to(cia402::State target) {
        // This is a simplified check - full implementation would verify all valid transitions
        update_state();

        // Can't transition if in fault (must reset first)
        if (current_state_ == cia402::State::FAULT && target != cia402::State::SWITCH_ON_DISABLED) {
            return false;
        }

        return true;
    }

    std::vector<cia402::State> CIA402FSM::get_transition_path(cia402::State target) {
        std::vector<cia402::State> path;

        // This would implement the full state transition graph
        // For now, return empty vector (to be implemented if needed)

        return path;
    }

    bool CIA402FSM::execute_transition(cia402::State target) {
        // This would execute a single state transition
        // For now, use the high-level methods

        return false;
    }

} // namespace canopen

/**
 * @file config.hpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief Configuration interface for state-first architecture
 * @version 3.0
 * @date 2025-10-09
 *
 * State-First Architecture:
 * - ConfigState holds baud_rate, can_mode, auto_rtx, filter, mask
 * - Setters modify state only (no buffer writes)
 * - Serialization happens on-demand
 *
 * @copyright Copyright (c) 2025
 */

#pragma once
#include "core.hpp"

namespace waveshare {

    /**
     * @brief Config state for configuration frames
     * Holds runtime values specific to config frames
     */
    struct ConfigState {
        CANBaud baud_rate = DEFAULT_CAN_BAUD;       // CAN bus baud rate
        CANMode can_mode = DEFAULT_CAN_MODE;        // CAN controller mode
        RTX auto_rtx = DEFAULT_RTX;                 // Automatic retransmission
        std::uint32_t filter = 0x00000000;          // Acceptance filter (29-bit max)
        std::uint32_t mask = 0x00000000;            // Acceptance mask (29-bit max)
    };

    /**
     * @brief Config interface for ConfigFrame (state-first design)
     *
     * This class provides:
     * - ConfigState holding all configuration parameters
     * - Getters/setters that modify state only
     * - Filter/mask support for CAN ID filtering
     *
     * @tparam Frame The frame type (ConfigFrame)
     */
    template<typename Frame>
    class ConfigInterface : public CoreInterface<Frame> {
        // Ensure that Frame is ConfigFrame
        static_assert(is_config_frame_v<Frame>,
            "Frame must be a config frame type (ConfigFrame)");

        protected:
            // * Config state (single source of truth)
            ConfigState config_state_;

            // * Prevent this class from being instantiated directly
            ConfigInterface() : CoreInterface<Frame>() {
                static_assert(!std::is_same_v<Frame, ConfigInterface>,
                    "ConfigInterface cannot be instantiated directly");
            }

        public:
            // === Configuration Methods ===

            /**
             * @brief Get the baud rate from state
             * @return CANBaud The current baud rate
             */
            CANBaud get_baud_rate() const {
                return config_state_.baud_rate;
            }

            /**
             * @brief Set the CAN bus baud rate in state
             * @param baud_rate The baud rate to set
             */
            void set_baud_rate(CANBaud baud_rate) {
                config_state_.baud_rate = baud_rate;
            }

            /**
             * @brief Get the CAN mode from state
             * @return CANMode The current CAN mode
             */
            CANMode get_can_mode() const {
                return config_state_.can_mode;
            }

            /**
             * @brief Set the CAN mode in state
             * @param mode The CAN mode to set
             */
            void set_can_mode(CANMode mode) {
                config_state_.can_mode = mode;
            }

            /**
             * @brief Get the acceptance filter from state
             * The filter determines which CAN IDs are accepted.
             * @note Filter range: 0x000-0x7FF for standard, 0x00000000-0x1FFFFFFF for extended
             * @return std::uint32_t The current acceptance filter
             */
            std::uint32_t get_filter() const {
                return config_state_.filter;
            }

            /**
             * @brief Set the acceptance filter in state
             * The filter determines which CAN IDs are accepted.
             * @note Filter range: 0x000-0x7FF for standard, 0x00000000-0x1FFFFFFF for extended
             * @param filter The acceptance filter to set
             */
            void set_filter(std::uint32_t filter) {
                config_state_.filter = filter;
            }

            /**
             * @brief Get the acceptance mask from state
             * The mask determines which bits of the CAN ID are relevant for filtering.
             * @note Mask range: 0x000-0x7FF for standard, 0x00000000-0x1FFFFFFF for extended
             * @return std::uint32_t The current acceptance mask
             */
            std::uint32_t get_mask() const {
                return config_state_.mask;
            }

            /**
             * @brief Set the acceptance mask in state
             * The mask determines which bits of the CAN ID are relevant for filtering.
             * @note Mask range: 0x000-0x7FF for standard, 0x00000000-0x1FFFFFFF for extended
             * @param mask The acceptance mask to set
             */
            void set_mask(std::uint32_t mask) {
                config_state_.mask = mask;
            }

            /**
             * @brief Get the automatic retransmission setting from state
             * @return RTX RTX::AUTO if enabled, RTX::OFF if disabled
             */
            RTX get_auto_rtx() const {
                return config_state_.auto_rtx;
            }

            /**
             * @brief Set automatic retransmission in state
             * @param auto_rtx RTX::AUTO to enable, RTX::OFF to disable
             */
            void set_auto_rtx(RTX auto_rtx) {
                config_state_.auto_rtx = auto_rtx;
            }

            /**
             * @brief Clear config-specific state
             * Called by CoreInterface::clear()
             */
            void impl_clear() {
                config_state_ = ConfigState{};
            }
    };
}

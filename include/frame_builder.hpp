/**
 * @file frame_builder.hpp
 * @brief Fluent builder pattern for type-safe frame construction.
 *
 * This file provides a builder pattern implementation that enables fluent,
 * validated frame construction with compile-time type safety. The builder
 * ensures that only valid operations are available for each frame type.
 *
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @version 1.0
 * @date 2025-09-29
 * @copyright Copyright (c) 2025
 */

#pragma once

#include "fixed_frame.hpp"
#include "variable_frame.hpp"
#include "config_frame.hpp"
#include <stdexcept>
#include <vector>

namespace USBCANBridge {

/**
 * @brief Fluent builder interface for frame construction.
 *
 * Provides a chainable interface for constructing frames with validation
 * at each step. Operations are compile-time restricted to appropriate
 * frame types using SFINAE.
 *
 * @tparam FrameType The type of frame to build (FixedFrame, VariableFrame, ConfigFrame)
 */

    template<typename Frame>
    class FrameBuilder {
        private:
            Frame frame_;

        public:
            FrameBuilder() = default;

            // ==== DATA FRAME OPERATIONS ====

            /**
             * @brief Set CAN ID (only available for data frames).
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, FrameBuilder&>
            can_id(uint32_t id) {
                auto result = frame_.set_can_id(id);
                if (!result) {
                    throw std::runtime_error("FrameBuilder: " + result.describe());
                }
                return *this;
            }

            /**
             * @brief Set data payload (only available for data frames).
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, FrameBuilder&>
            data(const std::vector<std::byte>& payload) {
                span<const std::byte> data_span(payload.data(), payload.size());
                auto result = frame_.set_data(data_span);
                if (!result) {
                    throw std::runtime_error("FrameBuilder: " + result.describe());
                }
                return *this;
            }

            /**
             * @brief Set data payload from raw bytes (only available for data frames).
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, FrameBuilder&>
            data(const uint8_t* bytes, std::size_t length) {
                span<const std::byte> data_span(reinterpret_cast<const std::byte*>(bytes), length);
                auto result = frame_.set_data(data_span);
                if (!result) {
                    throw std::runtime_error("FrameBuilder: " + result.describe());
                }
                return *this;
            }

            /**
             * @brief Set DLC (only available for data frames).
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, FrameBuilder&>
            dlc(uint8_t length) {
                auto result = frame_.set_dlc(std::byte{length});
                if (!result) {
                    throw std::runtime_error("FrameBuilder: " + result.describe());
                }
                return *this;
            }

            // ==== CONFIG FRAME OPERATIONS ====

            /**
             * @brief Set baud rate (only available for config frames).
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            baud_rate(CANBaud rate) {
                auto result = frame_.set_baud_rate(rate);
                if (!result) {
                    throw std::runtime_error("FrameBuilder: " + result.describe());
                }
                return *this;
            }

            /**
             * @brief Set filter and mask (only available for config frames).
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            filter(uint32_t filter) {
                auto result = frame_.set_filter(filter);
                if (!result) {
                    throw std::runtime_error("FrameBuilder: " + result.describe());
                }
                return *this;
            }
            /**
             * @brief Set ID mask (only available for config frames).
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            mask(uint32_t mask) {
                auto result = frame_.set_mask(mask);
                if (!result) {
                    throw std::runtime_error("FrameBuilder: " + result.describe());
                }
                return *this;
            }

            /**
             * @brief Set operating mode (only available for config frames).
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            mode(CANMode mode) {
                auto result = frame_.set_mode(mode);
                if (!result) {
                    throw std::runtime_error("FrameBuilder: " + result.describe());
                }
                return *this;
            }

            // ==== FIXED FRAME SPECIFIC OPERATIONS ====

            /**
             * @brief Set frame type (only available for FixedFrame).
             */
            template<typename T = Frame>
            std::enable_if_t<std::is_same_v<T, FixedFrame>, FrameBuilder&>
            frame_type(FrameType type) {
                auto result = frame_.set_frame_type(type);
                if (!result) {
                    throw std::runtime_error("FrameBuilder: " + result.describe());
                }
                return *this;
            }

            // ==== VARIABLE FRAME SPECIFIC OPERATIONS ====

            /**
             * @brief Set extended ID flag (only available for VariableFrame).
             */
            template<typename T = Frame>
            std::enable_if_t<std::is_same_v<T, VariableFrame>, FrameBuilder&>
            extended_id(bool extended = true) {
                auto result = frame_.set_extended_id(extended);
                if (!result) {
                    throw std::runtime_error("FrameBuilder: " + result.describe());
                }
                return *this;
            }

            // ==== FINALIZATION ====

            /**
             * @brief Build and validate the frame.
             *
             * @return The constructed and validated frame
             * @throws std::runtime_error if frame validation fails
             */
            Frame build() && {
                auto result = frame_.validate();
                if (!result || !result.value()) {
                    throw std::runtime_error("FrameBuilder: Frame validation failed - " +
                        (result ? "Invalid frame content" : result.describe()));
                }
                return std::move(frame_);
            }

            /**
             * @brief Build without move (allows reuse of builder).
             *
             * @return Copy of the constructed and validated frame
             * @throws std::runtime_error if frame validation fails
             */
            Frame build() const & {
                auto result = frame_.validate();
                if (!result || !result.value()) {
                    throw std::runtime_error("FrameBuilder: Frame validation failed - " +
                        (result ? "Invalid frame content" : result.describe()));
                }
                return frame_;
            }
    };

// ==== FACTORY FUNCTIONS ====

/**
 * @brief Create a FixedFrame builder.
 */
    inline FrameBuilder<FixedFrame> make_fixed_frame() {
        return FrameBuilder<FixedFrame>{};
    }

    inline FrameBuilder<VariableFrame> make_variable_frame() {
        return FrameBuilder<VariableFrame>{};
    }

    inline FrameBuilder<ConfigFrame> make_config_frame() {
        return FrameBuilder<ConfigFrame>{};
    }

    template<typename Frame>
    FrameBuilder<Frame> make_frame() {
        return FrameBuilder<Frame>{};
    }

} // namespace USBCANBridge
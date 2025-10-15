/**
 * @file frame_builder.hpp
 * @brief Fluent builder pattern for type-safe frame construction.
 *
 * This file provides a builder pattern implementation that enables fluent,
 * validated frame construction with compile-time type safety. The builder
 * ensures that only valid operations are available for each frame type.
 *
 * Key Features (v3.0 - State-First Architecture):
 * - Consistent with_* naming for all builder methods
 * - Automatic validation of CAN ID ranges and data sizes
 * - Move semantics support for efficient data transfers
 * - Initializer list support for convenient data assignment
 * - [[nodiscard]] attributes to prevent accidental misuse
 * - Automatic default value assignment in build()
 * - No separate finalize() step required
 * - SFINAE-based method restrictions per frame type
 *
 * Example Usage:
 * @code
 * // FixedFrame with fluent API
 * auto fixed = make_fixed_frame()
 *     .with_can_version(CANVersion::EXT_FIXED)
 *     .with_format(Format::DATA_FIXED)
 *     .with_id(0x12345678)
 *     .with_data({0x11, 0x22, 0x33})  // initializer list
 *     .build();
 *
 * // ConfigFrame with uint32_t filter/mask
 * auto config = make_config_frame()
 *     .with_baud_rate(CANBaud::BAUD_500K)
 *     .with_mode(CANMode::NORMAL)
 *     .with_filter(0x000007FF)  // uint32_t or array
 *     .build();
 *
 * // Move semantics for large data
 * std::vector<uint8_t> data = {0xAA, 0xBB, 0xCC};
 * auto var = make_variable_frame()
 *     .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
 *     .with_id(0x123)
 *     .with_data(std::move(data))  // efficient move
 *     .build();
 * @endcode
 *
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @version 3.0
 * @date 2025-10-10
 * @copyright Copyright (c) 2025
 */

#pragma once

#include "../frame/fixed_frame.hpp"
#include "../frame/variable_frame.hpp"
#include "../frame/config_frame.hpp"
#include <stdexcept>
#include <vector>
#include <optional>
#include "../enums/protocol.hpp"
#include "../template/frame_traits.hpp"

namespace waveshare {

/**
 * @brief Internal state holder for frame construction.
 * This separates the building state from the actual frame object.
 */
    template<typename Frame>
    struct FrameBuilderState {
        // Common fields
        std::optional<Type> type;
        std::optional<CANVersion> can_version;

        // Data frame fields
        std::optional<Format> format;
        std::optional<uint32_t> id;
        std::optional<std::vector<std::uint8_t> > data;

        // Config frame fields
        std::optional<CANBaud> baud_rate;
        std::optional<CANMode> mode;
        std::optional<std::array<std::uint8_t, 4> > filter;
        std::optional<std::array<std::uint8_t, 4> > mask;
        std::optional<RTX> auto_rtx;
    };

/**
 * @brief Fluent builder interface for frame construction.
 *
 * Provides a chainable interface for constructing frames with validation
 * at each step. Operations are compile-time restricted to appropriate
 * frame types using SFINAE.
 *
 * Features:
 * - All methods use with_* naming convention for consistency
 * - [[nodiscard]] attributes prevent ignoring builder returns
 * - Automatic validation of ID ranges and data sizes
 * - Move semantics and initializer list support for data
 * - No separate finalize() step - build() handles defaults
 * - Type-safe method availability via SFINAE
 *
 * @tparam Frame The type of frame to build (FixedFrame, VariableFrame, ConfigFrame)
 */
    template<typename Frame>
    class FrameBuilder {
        using traits_t = FrameTraits<Frame>;

        private:
            FrameBuilderState<Frame> state_;

            /**
             * @brief Validate that all required fields are set before building.
             * Sets defaults for optional fields and validates required fields.
             * @throws std::runtime_error if required fields are missing
             */
            void validate_and_set_defaults() {
                // Set defaults for optional fields based on frame type
                if constexpr (is_data_frame_v<Frame>) {
                    if (!state_.can_version) {
                        state_.can_version = CANVersion::STD_FIXED;
                    }
                    if (!state_.format) {
                        state_.format = Format::DATA_FIXED;
                    }

                    // Validate required fields
                    if (!state_.id) {
                        throw std::runtime_error("CAN ID not set");
                    }
                    // Data is optional (remote frames may have no data)
                }

                if constexpr (is_config_frame_v<Frame>) {
                    if (!state_.can_version) {
                        state_.can_version = CANVersion::STD_FIXED;
                    }
                    if (!state_.type) {
                        state_.type = DEFAULT_CONF_TYPE;
                    }
                    if (!state_.auto_rtx) {
                        state_.auto_rtx = RTX::AUTO;
                    }

                    // Validate required fields
                    if (!state_.baud_rate) {
                        throw std::runtime_error("Baud rate not set");
                    }
                    if (!state_.mode) {
                        throw std::runtime_error("CAN mode not set");
                    }
                    // Filter and mask are optional (default to 0)
                }
            }

            /**
             * @brief Construct the actual frame from the builder state.
             * @return The constructed frame
             */
            Frame construct_frame() const {
                if constexpr (is_fixed_frame_v<Frame>) {
                    // Use FixedFrame constructor directly
                    std::vector<std::uint8_t> data_vec =
                        state_.data.value_or(std::vector<std::uint8_t>{});
                    Frame frame(
                        state_.format.value(),
                        state_.can_version.value(),
                        state_.id.value(),
                        span<const std::uint8_t>(data_vec.data(), data_vec.size())
                    );
                    return frame;

                } else if constexpr (is_variable_frame_v<Frame>) {
                    // Use VariableFrame constructor directly
                    std::vector<std::uint8_t> data_vec =
                        state_.data.value_or(std::vector<std::uint8_t>{});
                    Frame frame(
                        state_.format.value(),
                        state_.can_version.value(),
                        state_.id.value(),
                        span<const std::uint8_t>(data_vec.data(), data_vec.size())
                    );
                    return frame;

                } else if constexpr (is_config_frame_v<Frame>) {
                    // Extract filter and mask as uint32_t from optional arrays
                    std::uint32_t filter_val = 0;
                    std::uint32_t mask_val = 0;

                    if (state_.filter.has_value()) {
                        const auto& f = state_.filter.value();
                        filter_val = (static_cast<std::uint32_t>(f[3]) << 24) |
                            (static_cast<std::uint32_t>(f[2]) << 16) |
                            (static_cast<std::uint32_t>(f[1]) << 8) |
                            static_cast<std::uint32_t>(f[0]);
                    }

                    if (state_.mask.has_value()) {
                        const auto& m = state_.mask.value();
                        mask_val = (static_cast<std::uint32_t>(m[3]) << 24) |
                            (static_cast<std::uint32_t>(m[2]) << 16) |
                            (static_cast<std::uint32_t>(m[1]) << 8) |
                            static_cast<std::uint32_t>(m[0]);
                    }

                    Frame frame(
                        state_.type.value_or(DEFAULT_CONF_TYPE),
                        state_.baud_rate.value(),
                        state_.mode.value(),
                        state_.auto_rtx.value_or(RTX::AUTO),
                        filter_val,
                        mask_val,
                        state_.can_version.value()
                    );
                    return frame;
                }
            }

        public:
            FrameBuilder() = default;

            // === Core Methods ===

            /**
             * @brief Set the Type of the frame.
             * @param type The type of the frame
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            [[nodiscard]] std::enable_if_t<!is_variable_frame_v<T>, FrameBuilder&>
            with_type(Type type) {
                state_.type = type;
                return *this;
            }

            /**
             * @brief Set the Type of the frame for variable frames.
             * @param ver The CAN version (standard/extended)
             * @param fmt The frame format (data/remote)
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            [[nodiscard]] std::enable_if_t<is_variable_frame_v<T>, FrameBuilder&>
            with_type(CANVersion ver, Format fmt) {
                state_.can_version = ver;
                state_.format = fmt;
                return *this;
            }

            /**
             * @brief Set the CAN version for the frame.
             * @param ver The CAN version (standard/extended)
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            [[nodiscard]] std::enable_if_t<!is_variable_frame_v<T>, FrameBuilder&>
            with_can_version(CANVersion ver) {
                state_.can_version = ver;
                return *this;
            }

            // === Data Frame Methods ===

            /**
             * @brief Set the frame format for data frames.
             * @param format The frame format (data/remote)
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            [[nodiscard]] std::enable_if_t<is_data_frame_v<T>&& !is_variable_frame_v<T>,
                FrameBuilder&>
            with_format(Format format) {
                state_.format = format;
                return *this;
            }

            /**
             * @brief Set the CAN ID for data frames.
             * @param id The CAN ID to set
             * @return Reference to the builder for chaining.
             * @note Validation happens during build() via frame constructor
             */
            template<typename T = Frame>
            [[nodiscard]] std::enable_if_t<is_data_frame_v<T>, FrameBuilder&>
            with_id(uint32_t id) {
                state_.id = id;
                return *this;
            }

            /**
             * @brief Set the data payload for data frames (copy version).
             * @param data A vector of bytes representing the data payload.
             * @return Reference to the builder for chaining.
             * @note Validation happens in frame's set_data() during build()
             */
            template<typename T = Frame>
            [[nodiscard]] std::enable_if_t<is_data_frame_v<T>, FrameBuilder&>
            with_data(const std::vector<std::uint8_t>& data) {
                state_.data = data;
                return *this;
            }

            /**
             * @brief Set the data payload for data frames (move version).
             * @param data A vector of bytes representing the data payload (moved).
             * @return Reference to the builder for chaining.
             * @note Validation happens in frame's set_data() during build()
             */
            template<typename T = Frame>
            [[nodiscard]] std::enable_if_t<is_data_frame_v<T>, FrameBuilder&>
            with_data(std::vector<std::uint8_t>&& data) {
                state_.data = std::move(data);
                return *this;
            }

            /**
             * @brief Set the data payload for data frames (initializer list).
             * @param data Initializer list of bytes.
             * @return Reference to the builder for chaining.
             * @note Validation happens in frame's set_data() during build()
             */
            template<typename T = Frame>
            [[nodiscard]] std::enable_if_t<is_data_frame_v<T>, FrameBuilder&>
            with_data(std::initializer_list<std::uint8_t> data) {
                state_.data = std::vector<std::uint8_t>(data);
                return *this;
            }

            // === Config Frame Methods ===

            /**
             * @brief Set the CAN baud rate for config frames.
             * @param baud_rate The CAN baud rate to set.
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            [[nodiscard]] std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            with_baud_rate(CANBaud baud_rate) {
                state_.baud_rate = baud_rate;
                return *this;
            }

            /**
             * @brief Set the CAN mode for config frames.
             * @param mode The CAN mode to set.
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            [[nodiscard]] std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            with_mode(CANMode mode) {
                state_.mode = mode;
                return *this;
            }

            /**
             * @brief Set the acceptance filter for config frames (array version).
             * @param filter A 4-byte array representing the acceptance filter.
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            [[nodiscard]] std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            with_filter(const std::array<std::uint8_t, 4>& filter) {
                state_.filter = filter;
                return *this;
            }

            /**
             * @brief Set the acceptance filter for config frames (uint32_t version).
             * @param filter A 32-bit value representing the acceptance filter.
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            [[nodiscard]] std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            with_filter(std::uint32_t filter) {
                std::array<std::uint8_t, 4> filter_arr;
                filter_arr[0] = static_cast<std::uint8_t>(filter & 0xFF);
                filter_arr[1] = static_cast<std::uint8_t>((filter >> 8) & 0xFF);
                filter_arr[2] = static_cast<std::uint8_t>((filter >> 16) & 0xFF);
                filter_arr[3] = static_cast<std::uint8_t>((filter >> 24) & 0xFF);
                state_.filter = filter_arr;
                return *this;
            }

            /**
             * @brief Set the acceptance mask for config frames (array version).
             * @param mask A 4-byte array representing the acceptance mask.
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            [[nodiscard]] std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            with_mask(const std::array<std::uint8_t, 4>& mask) {
                state_.mask = mask;
                return *this;
            }

            /**
             * @brief Set the acceptance mask for config frames (uint32_t version).
             * @param mask A 32-bit value representing the acceptance mask.
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            [[nodiscard]] std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            with_mask(std::uint32_t mask) {
                std::array<std::uint8_t, 4> mask_arr;
                mask_arr[0] = static_cast<std::uint8_t>(mask & 0xFF);
                mask_arr[1] = static_cast<std::uint8_t>((mask >> 8) & 0xFF);
                mask_arr[2] = static_cast<std::uint8_t>((mask >> 16) & 0xFF);
                mask_arr[3] = static_cast<std::uint8_t>((mask >> 24) & 0xFF);
                state_.mask = mask_arr;
                return *this;
            }

            /**
             * @brief Set the auto retransmission setting for config frames.
             * @param auto_rtx The auto retransmission setting to set.
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            [[nodiscard]] std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            with_rtx(RTX auto_rtx) {
                state_.auto_rtx = auto_rtx;
                return *this;
            }

            // ==== BUILD METHOD ====

            /**
             * @brief Build and validate the frame (move semantics).
             * Automatically sets defaults for optional fields and validates required fields.
             * @return The constructed and validated frame
             * @throws std::runtime_error if required fields are missing
             * @throws std::invalid_argument if field values are invalid
             */
            Frame build() && {
                validate_and_set_defaults();
                Frame frame = construct_frame();
                return frame;
            }

            /**
             * @brief Build without move (allows reuse of builder).
             * Automatically sets defaults for optional fields and validates required fields.
             * @return Copy of the constructed and validated frame
             * @throws std::runtime_error if required fields are missing
             * @throws std::invalid_argument if field values are invalid
             */
            Frame build() const & {
                // Create mutable copy for validation
                auto mutable_state = state_;
                FrameBuilder<Frame> temp_builder;
                temp_builder.state_ = mutable_state;
                temp_builder.validate_and_set_defaults();
                return temp_builder.construct_frame();
            }
    };

// ==== FACTORY FUNCTIONS ====

    /**
     * @brief Create a FixedFrame builder.
     *
     * Factory function for building 20-byte fixed frames with automatic
     * checksum calculation and little-endian ID encoding.
     *
     * @return FrameBuilder<FixedFrame> A new builder instance
     *
     * @example
     * auto frame = make_fixed_frame()
     *     .with_can_version(CANVersion::STD_FIXED)
     *     .with_id(0x123)
     *     .with_data({0x11, 0x22})
     *     .build();
     */
    inline FrameBuilder<FixedFrame> make_fixed_frame() {
        return FrameBuilder<FixedFrame>{};
    }

    /**
     * @brief Create a VariableFrame builder.
     *
     * Factory function for building 5-15 byte variable frames with
     * dynamic TYPE byte encoding based on CAN version, format, and DLC.
     *
     * @return FrameBuilder<VariableFrame> A new builder instance
     *
     * @example
     * auto frame = make_variable_frame()
     *     .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
     *     .with_id(0x456)
     *     .with_data({0xAA, 0xBB, 0xCC})
     *     .build();
     */
    inline FrameBuilder<VariableFrame> make_variable_frame() {
        return FrameBuilder<VariableFrame>{};
    }

    /**
     * @brief Create a ConfigFrame builder.
     *
     * Factory function for building 20-byte configuration frames to
     * set up the USB-CAN adapter (baud rate, mode, filters, etc.).
     *
     * @return FrameBuilder<ConfigFrame> A new builder instance
     *
     * @example
     * auto config = make_config_frame()
     *     .with_baud_rate(CANBaud::BAUD_1M)
     *     .with_mode(CANMode::NORMAL)
     *     .with_filter(0x7FF)  // uint32_t or array
     *     .build();
     */
    inline FrameBuilder<ConfigFrame> make_config_frame() {
        return FrameBuilder<ConfigFrame>{};
    }

    /**
     * @brief Create a generic frame builder.
     *
     * Template factory function for building any frame type.
     * Prefer using specific factories (make_fixed_frame, etc.) for clarity.
     *
     * @tparam Frame The frame type (FixedFrame, VariableFrame, ConfigFrame)
     * @return FrameBuilder<Frame> A new builder instance
     */
    template<typename Frame>
    FrameBuilder<Frame> make_frame() {
        return FrameBuilder<Frame>{};
    }

} // namespace waveshare
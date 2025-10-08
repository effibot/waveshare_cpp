/**
 * @file frame_builder.hpp
 * @brief Fluent builder pattern for type-safe frame construction.
 *
 * This file provides a builder pattern implementation that enables fluent,
 * validated frame construction with compile-time type safety. The builder
 * ensures that only valid operations are available for each frame type.
 *
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @version 2.0
 * @date 2025-10-08
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

namespace USBCANBridge {

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
        std::optional<std::vector<std::byte> > data;

        // Config frame fields
        std::optional<CANBaud> baud_rate;
        std::optional<CANMode> mode;
        std::optional<std::array<std::byte, 4> > filter;
        std::optional<std::array<std::byte, 4> > mask;
        std::optional<RTX> auto_rtx;

        // Builder state
        bool finalized = false;
    };

/**
 * @brief Fluent builder interface for frame construction.
 *
 * Provides a chainable interface for constructing frames with validation
 * at each step. Operations are compile-time restricted to appropriate
 * frame types using SFINAE.
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
             * @throws std::runtime_error if required fields are missing
             */
            void validate_state() const {
                if (!state_.finalized) {
                    throw std::runtime_error(
                        "Builder not finalized - call finalize() before build()");
                }

                if constexpr (is_data_frame_v<Frame>) {
                    if (!state_.can_version) {
                        throw std::runtime_error("CAN version not set");
                    }
                    if (!state_.format) {
                        throw std::runtime_error("Format not set");
                    }
                    if (!state_.id) {
                        throw std::runtime_error("CAN ID not set");
                    }
                    // Data is optional (remote frames may have no data)
                }

                if constexpr (is_config_frame_v<Frame>) {
                    if (!state_.can_version) {
                        throw std::runtime_error("CAN version not set");
                    }
                    if (!state_.baud_rate) {
                        throw std::runtime_error("Baud rate not set");
                    }
                    if (!state_.mode) {
                        throw std::runtime_error("CAN mode not set");
                    }
                    // Filter, mask, and auto_rtx are optional (have defaults)
                }
            }

            /**
             * @brief Construct the actual frame from the builder state.
             * @return The constructed frame
             */
            Frame construct_frame() const {
                if constexpr (is_fixed_frame_v<Frame>) {
                    return construct_fixed_frame();
                } else if constexpr (is_variable_frame_v<Frame>) {
                    return construct_variable_frame();
                } else if constexpr (is_config_frame_v<Frame>) {
                    return construct_config_frame();
                }
            }

            Frame construct_fixed_frame() const {
                Frame frame(
                    state_.format.value(),
                    state_.can_version.value(),
                    {}, // ID will be set below
                    state_.data ? state_.data->size() : 0,
                    {} // Data will be set below
                );

                frame.set_id(state_.id.value());

                if (state_.data) {
                    frame.set_data(span<const std::byte>(state_.data->data(), state_.data->size()));
                }

                return frame;
            }

            Frame construct_variable_frame() const {
                std::vector<std::byte> init_id;
                std::size_t id_size = (state_.can_version.value() ==
                    CANVersion::EXT_VARIABLE) ? 4 : 2;

                if (state_.id) {
                    uint32_t id_val = state_.id.value();
                    init_id.resize(id_size);
                    for (std::size_t i = 0; i < id_size; ++i) {
                        init_id[i] = static_cast<std::byte>((id_val >> (i * 8)) & 0xFF);
                    }
                }

                Frame frame(
                    state_.format.value(),
                    state_.can_version.value(),
                    init_id,
                    state_.data ? state_.data->size() : 0,
                    state_.data ? *state_.data : std::vector<std::byte>{}
                );

                return frame;
            }

            Frame construct_config_frame() const {
                Frame frame(
                    state_.type.value_or(DEFAULT_CONF_TYPE),
                    state_.filter.value_or(std::array<std::byte, 4>{}),
                    state_.mask.value_or(std::array<std::byte, 4>{}),
                    state_.auto_rtx.value_or(RTX::AUTO),
                    state_.baud_rate.value(),
                    state_.mode.value()
                );

                frame.set_CAN_version(state_.can_version.value());

                return frame;
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
            std::enable_if_t<!is_variable_frame_v<T>, FrameBuilder&>
            type(Type type) {
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
            std::enable_if_t<is_variable_frame_v<T>, FrameBuilder&>
            type(CANVersion ver, Format fmt) {
                state_.can_version = ver;
                state_.format = fmt;
                return *this;
            }

            /**
             * @brief Set the CAN version for the frame.
             * @param frame_type The CAN version (standard/extended)
             * @return Reference to the builder for chaining.
             */
            FrameBuilder& can_version(CANVersion frame_type) {
                state_.can_version = frame_type;
                return *this;
            }

            // === Data Frame Methods ===

            /**
             * @brief Set the frame format for data frames.
             * @param format The frame format (data/remote)
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, FrameBuilder&>
            format(Format format) {
                state_.format = format;
                return *this;
            }

            /**
             * @brief Set the CAN ID for data frames.
             * @param id The CAN ID to set
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, FrameBuilder&>
            id(uint32_t id) {
                state_.id = id;
                return *this;
            }

            /**
             * @brief Set the data payload for data frames.
             * @param data A vector of bytes representing the data payload.
             * @return Reference to the builder for chaining.
             * @throws std::out_of_range if the data size exceeds the maximum allowed.
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, FrameBuilder&>
            data(const std::vector<std::byte>& data) {
                if (data.size() > traits_t::MAX_DATA_SIZE) {
                    throw std::out_of_range("Data size exceeds maximum for frame");
                }
                state_.data = data;
                return *this;
            }

            // === Config Frame Methods ===

            /**
             * @brief Set the CAN baud rate for config frames.
             * @param baud_rate The CAN baud rate to set.
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            baud_rate(CANBaud baud_rate) {
                state_.baud_rate = baud_rate;
                return *this;
            }

            /**
             * @brief Set the CAN mode for config frames.
             * @param mode The CAN mode to set.
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            mode(CANMode mode) {
                state_.mode = mode;
                return *this;
            }

            /**
             * @brief Set the acceptance filter for config frames.
             * @param filter A 4-byte array representing the acceptance filter.
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            filter(const std::array<std::byte, 4>& filter) {
                state_.filter = filter;
                return *this;
            }

            /**
             * @brief Set the acceptance mask for config frames.
             * @param mask A 4-byte array representing the acceptance mask.
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            mask(const std::array<std::byte, 4>& mask) {
                state_.mask = mask;
                return *this;
            }

            /**
             * @brief Set the auto retransmission setting for config frames.
             * @param auto_rtx The auto retransmission setting to set.
             * @return Reference to the builder for chaining.
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
            rtx(RTX auto_rtx) {
                state_.auto_rtx = auto_rtx;
                return *this;
            }

            // ==== FINALIZATION ====

            /**
             * @brief Mark the builder as ready for frame construction.
             * This validates that all required fields are set and allows build() to proceed.
             * @return Reference to the builder for chaining.
             * @throws std::runtime_error if required fields are missing
             */
            FrameBuilder& finalize() {
                // Set defaults for optional fields based on frame type
                if constexpr (is_data_frame_v<Frame>) {
                    if (!state_.can_version) {
                        state_.can_version = CANVersion::STD_FIXED;
                    }
                    if (!state_.format) {
                        state_.format = Format::DATA_FIXED;
                    }
                }

                if constexpr (is_config_frame_v<Frame>) {
                    if (!state_.type) {
                        state_.type = DEFAULT_CONF_TYPE;
                    }
                }

                state_.finalized = true;
                return *this;
            }

            /**
             * @brief Check if the builder has been finalized.
             * @return true if finalized, false otherwise
             */
            bool is_finalized() const {
                return state_.finalized;
            }

            /**
             * @brief Build and validate the frame (move semantics).
             * @return The constructed and validated frame
             * @throws std::runtime_error if frame validation fails
             */
            Frame build() && {
                validate_state();
                Frame frame = construct_frame();
                frame.finalize();
                return frame;
            }

            /**
             * @brief Build without move (allows reuse of builder).
             * @return Copy of the constructed and validated frame
             * @throws std::runtime_error if frame validation fails
             */
            Frame build() const & {
                validate_state();
                Frame frame = construct_frame();
                frame.finalize();
                return frame;
            }
    };

// ==== FACTORY FUNCTIONS ====

    /**
     * @brief Create a FixedFrame builder.
     * @tparam FixedFrame The type of frame to build (FixedFrame, VariableFrame, ConfigFrame)
     * @return FrameBuilder<FixedFrame>
     */
    inline FrameBuilder<FixedFrame> make_fixed_frame() {
        return FrameBuilder<FixedFrame>{};
    }
    /**
     * @brief Create a VariableFrame builder.
     * @tparam VariableFrame The type of frame to build (FixedFrame, VariableFrame, ConfigFrame)
     * @return FrameBuilder<VariableFrame>
     */
    inline FrameBuilder<VariableFrame> make_variable_frame() {
        return FrameBuilder<VariableFrame>{};
    }
    /**
     * @brief Create a ConfigFrame builder.
     * @tparam ConfigFrame The type of frame to build (FixedFrame, VariableFrame, ConfigFrame)
     * @return FrameBuilder<ConfigFrame>
     */
    inline FrameBuilder<ConfigFrame> make_config_frame() {
        return FrameBuilder<ConfigFrame>{};
    }
    /**
     * @brief Create a generic frame builder.
     * @tparam Frame The type of frame to build (FixedFrame, VariableFrame, ConfigFrame)
     * @return FrameBuilder<Frame>
     */
    template<typename Frame>
    FrameBuilder<Frame> make_frame() {
        return FrameBuilder<Frame>{};
    }

} // namespace USBCANBridge
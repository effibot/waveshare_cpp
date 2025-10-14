/**
 * @file core.hpp
 * @author Andrea Efficace (andrea.efficace@)
 * @brief Core interface for state-first frame architecture
 * @version 4.0
 * @date 2025-10-14
 *
 * State-First Architecture:
 * - CoreState holds runtime frame state (can_version, type)
 * - No persistent buffer storage
 * - Serialization on-demand via serialize()
 * - FrameTraits provides compile-time layout metadata
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include <boost/core/span.hpp>
#include <cstddef>
#include <type_traits>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

#include "../enums/protocol.hpp"
#include "../exception/waveshare_exception.hpp"
#include "../template/result.hpp"  // TODO: Remove after migration complete
#include "../template/frame_traits.hpp"

using namespace boost;


namespace waveshare {

    /**
     * @brief Core state for all frame types
     * Holds runtime values common to all frames
     */
    struct CoreState {
        CANVersion can_version = CANVersion::STD_VARIABLE;  // Standard or Extended CAN
        Type type = Type::DATA_VARIABLE;                    // Frame type (DATA_FIXED, DATA_VARIABLE, CONF_FIXED, CONF_VARIABLE)
    };

    /**
     * @brief Core interface for all frame types (state-first design)
     *
     * This class provides:
     * - CoreState holding can_version and type
     * - Serialization/deserialization methods
     * - Common getters/setters that modify state only
     * - CRTP access to derived frame implementations
     *
     * @tparam Frame The frame type to interface with (FixedFrame, VariableFrame, ConfigFrame)
     */
    template<typename Frame>
    class CoreInterface {

        protected:
            // * Core state (single source of truth)
            CoreState core_state_;

            // * CRTP helper to access derived class methods
            Frame& derived() {
                return static_cast<Frame&>(*this);
            }

            // * CRTP helper to access derived class methods (const version)
            const Frame& derived() const {
                return static_cast<const Frame&>(*this);
            }

            // * Default constructor
            CoreInterface() {
                static_assert(!std::is_same_v<Frame, CoreInterface>,
                    "CoreInterface cannot be instantiated directly");
            }

        public:

            // === Serialization Methods ===

            /**
             * @brief Serialize the frame state to a byte buffer
             *
             * Creates a transient buffer from the current state using layout metadata.
             * This is the primary method to get the wire-format representation.
             *
             * @return std::vector<std::uint8_t> Serialized frame buffer
             * @note Calls derived().impl_serialize() for frame-specific logic
             */
            std::vector<std::uint8_t> serialize() const {
                return derived().impl_serialize();
            }

            /**
             * @brief Deserialize a byte buffer into frame state
             *
             * Populates state from a wire-format buffer.
             * Throws exception on validation errors.
             *
             * @param buffer Byte buffer to deserialize
             * @throws ProtocolException on validation errors
             * @throws DeviceException on I/O errors
             * @note Calls derived().impl_deserialize() for frame-specific logic
             */
            void deserialize(span<const std::uint8_t> buffer) {
                derived().impl_deserialize(buffer);
            }

            /**
             * @brief Deserialize with Result<void> (deprecated, for backward compatibility)
             *
             * @deprecated Use exception-throwing deserialize() instead
             * @param buffer Byte buffer to deserialize
             * @return Result<void> Success or error status
             */
            [[deprecated("Use exception-throwing deserialize() instead")]]
            Result<void> deserialize_result(span<const std::uint8_t> buffer) {
                try {
                    derived().impl_deserialize(buffer);
                    return Result<void>::success();
                } catch (const WaveshareException& e) {
                    return Result<void>::error(e.status(), e.context());
                }
            }

            /**
             * @brief Get the serialized size of this frame
             *
             * For fixed frames, returns constant size.
             * For variable frames, computes size based on current state.
             *
             * @return std::size_t Size in bytes
             * @note Calls derived().impl_serialized_size() for frame-specific logic
             */
            std::size_t serialized_size() const {
                return derived().impl_serialized_size();
            }

            // === State Access Methods ===

            /**
             * @brief Get the CANVersion (Standard or Extended)
             * @return CANVersion Current CAN version from state
             */
            CANVersion get_CAN_version() const {
                return core_state_.can_version;
            }

            /**
             * @brief Set the CANVersion (Standard or Extended)
             * @param version The CANVersion to set
             */
            void set_CAN_version(CANVersion version) {
                core_state_.can_version = version;
            }

            /**
             * @brief Get the frame Type
             * @return Type Current frame type from state
             */
            Type get_type() const {
                return core_state_.type;
            }

            /**
             * @brief Set the frame Type (for non-variable frames)
             * @param type The Type to set
             */
            template<typename T = Frame>
            std::enable_if_t<!is_variable_frame_v<T>, void>
            set_type(Type type) {
                core_state_.type = type;
            }

            // === Utility Methods ===

            /**
             * @brief Clear the frame state
             * Resets to default values
             */
            void clear() {
                core_state_ = CoreState{};
                // Let derived classes clear their specific state
                derived().impl_clear();
            }

            /**
             * @brief Print the frame in a human-readable format
             * @return std::string A string representation of the frame
             */
            std::string to_string() const {
                auto buffer = serialize();
                // Convert buffer to hex string
                std::ostringstream oss;
                oss << std::hex << std::setfill('0');
                for (const auto& byte : buffer) {
                    oss << std::setw(2) << static_cast<int>(byte) << " ";
                }
                std::string result = oss.str();
                if (!result.empty()) {
                    result.pop_back(); // Remove trailing space
                }
                return result;
            }

            /**
             * @brief Get the size of the frame in bytes
             * Alias for serialized_size()
             * @return std::size_t The size of the frame in bytes
             */
            std::size_t size() const {
                return serialized_size();
            }


    };
}


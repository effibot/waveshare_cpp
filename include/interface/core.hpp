/**
 * @file core.hpp
 * @author Andrea Efficace (andrea.efficace@)
 * @brief
 * @version 0.1
 * @date 2025-10-02
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include <boost/core/span.hpp>
#include <cstddef>
#include <type_traits>

#include "../enums/protocol.hpp"
#include "../template/result.hpp"
#include "../template/frame_traits.hpp"

#include "core_validator.hpp"
#include "checksum.hpp"

using namespace boost;

namespace USBCANBridge {
    /**
     * @brief Core interface for all frame types
     * This class provides common methods and utilities for all frame types.
     * @tparam Frame The frame type to interface with.
     */
    template<typename Frame>
    class CoreInterface {
        // * Alias for traits
        using traits = frame_traits_t<Frame>;
        using layout = layout_t<Frame>;
        using storage = storage_t<Frame>;

        protected:
            // * Storage for the frame data
            alignas(8) mutable storage frame_storage_{};

            // * CRTP helper to access derived class methods
            Frame& derived() {
                return static_cast<Frame&>(*this);
            }

            // * CRTP helper to access derived class methods (const version)
            const Frame& derived() const {
                return static_cast<const Frame&>(*this);
            }

            // * Prevent this class from being instantiated directly
            CoreInterface() {
                static_assert(!std::is_same_v<Frame, CoreInterface>,
                    "CoreInterface cannot be instantiated directly");
                init_fields();
            }

        private:

            /**
             * @brief Initialize constants fields in the frame storage.
             * This method is called by the derived class constructor (or by the `clear()`) method to set (or reset) the initial values of the constant fields.
             */
            void init_fields() {
                // Set the Start byte
                frame_storage_[layout::START_OFFSET] = to_byte(Constants::START_BYTE);
                // Call derived class to initialize fixed fields
                derived().impl_init_fields();
                return;
            }

        public:

            // === Common Frame Methods ===

            /**
             * @brief Serialize the frame to a byte array.
             * Convert the frame into a read-only byte array for transmission.
             *
             * @note This is common for all frame types and does't call derived().
             * @return Result<span<std::byte> > A span representing the serialized frame data. The length of the span is fixed to FRAME_SIZE for fixed-size frames, or the current size for variable-size frames.
             */
            template<typename T = Frame, size_t N>
            Result<span<const std::byte, N> > serialize() const {
                size_t current_size;
                if constexpr (!is_variable_frame_v<T>) {
                    current_size = layout::FRAME_SIZE;
                } else {
                    current_size = derived().impl_size().value();
                }
                return Result<span<const std::byte, N> >(span<const std::byte,
                    N>(frame_storage_.data(), current_size));
            }


            /**
             * @brief Serialize the frame to a writable byte array.
             * @note This is common for all frame types and does't call derived().
             * @return Result<span<std::byte> > A span representing the serialized frame data. The length of the span is fixed to FRAME_SIZE for fixed-size frames, or the current size for variable-size frames.
             */
            template<typename T = Frame, size_t N>
            Result<span<std::byte, N> > serialize() {
                // * Determine current size
                size_t current_size = size().value();
                // * Return the serialized frame data
                return Result<span<std::byte, N> >(span<std::byte, N>(
                    frame_storage_.data(),
                    current_size
                ));
            }

            /**
             * @brief Deserialize the frame from a byte array.
             * @param data A span representing the serialized frame data.
             * @return Result<Status> Status::SUCCESS on success, or an error status on failure.
             * @note This calls derived().deserialize() for frame-specific deserialization.
             */
            Result<void> deserialize(span<const std::byte> data) {
                // * call derived class to perform deserialization
                auto result = derived().impl_deserialize(data);
                if (!result) {
                    return Result<void>::error(result.error(), "CoreInterface::deserialize");
                }
                return Result<void>::success();
            }

            /**
             * @brief Clear the frame data.
             * Reset the frame to its initial state.
             * @return Result<Status> Status::SUCCESS on success, or an error status on failure.
             * @note This will zero out the frame storage and re-initialize constant fields.
             */
            Result<void> clear() {
                std::fill(std::begin(frame_storage_), std::end(frame_storage_), 0x00);
                init_fields();
                return Result<void>::success();
            }

            /**
             * @brief Print the frame in a human-readable format.
             * @return Result<std::string> A string representation of the frame.
             * @note This calls derived().to_string() for frame-specific string conversion.
             */
            Result<std::string> to_string() const {
                return dump_frame<Frame>(frame_storage_);
            }
            /**
             * @brief Get the size of the frame in bytes.
             * @note If the frame has a fixed size, this returns that size. If variable, it calls derived() to compute the size.
             * @return Result<std::size_t> The size of the frame in bytes.
             */
            template<typename T = Frame>
            Result<std::size_t> size() const {
                if  constexpr (!is_variable_frame_v<T>) {
                    return Result<std::size_t>(traits::FRAME_SIZE);
                }
                return derived().impl_size();

            }

            // === Protocol-Specific Methods ===
            /**
             * @brief Get the Type byte of the frame.
             * @return Result<Type> The Type byte of the frame.
             * @note This calls derived().get_type() for frame-specific type retrieval.
             */
            Result<Type> get_type() const {
                return derived().impl_get_type();
            }
            /**
             * @brief Set the Type byte of the frame.
             * @param type The Type byte to set.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             * @note This calls derived().set_type() for frame-specific type setting.
             */
            template<typename T = Frame>
            std::enable_if_t<!is_variable_frame_v<T>, Result<void> >
            set_type(Type type) {
                return derived().impl_set_type(type);
            }
            /**
             * @brief Set the Type byte of the frame.
             * @return Result<Type> The Type byte of the frame.
             * @note This calls derived().get_type() for frame-specific type retrieval.
             */
            template<typename T = Frame>
            std::enable_if_t<is_variable_frame_v<T>, Result<void> >
            set_type(std::byte type) {
                return derived().impl_set_type(type);
            }

            /**
             * @brief Get the FrameType byte of the frame.
             * @return Result<FrameType> The FrameType byte of the frame.
             * @note This calls derived().get_frame_type() for frame-specific frame type retrieval.
             */
            Result<FrameType> get_frame_type() const {
                return derived().impl_get_frame_type();
            }
            /**
             * @brief Set the FrameType byte of the frame.
             * @param frame_type The FrameType byte to set.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             * @note This calls derived().set_frame_type() for frame-specific frame type setting.
             */
            template<typename T = Frame>
            std::enable_if_t<is_variable_frame_v<T>, Result<void> >
            set_frame_type(FrameType frame_type) {
                return derived().impl_set_frame_type(frame_type);
            }
    };
}


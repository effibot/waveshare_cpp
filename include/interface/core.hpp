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

using namespace boost;

namespace USBCANBridge {


    template<typename Derived>
    class CoreInterface : public CoreValidator<Derived> {
        // * Alias for traits
        using traits = frame_traits_t<Derived>;
        using layout = layout_t<Derived>;
        using storage = storage_t<Derived>;

        protected:

            // * CRTP helper to access derived class methods
            Derived& derived() {
                return static_cast<Derived&>(*this);
            }
            // * CRTP helper to access derived class methods (const version)
            const Derived& derived() const {
                return static_cast<const Derived&>(*this);
            }
            // * Storage for the frame data
            alignas(8) mutable storage frame_storage_{};

            // * Prevent this class from being instantiated directly
            CoreInterface() {
                static_assert(!std::is_same_v<Derived, CoreInterface>,
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
            template<typename T = Derived, size_t N>
            Result<span<const std::byte, N> > serialize() const {
                size_t current_size;
                if constexpr (!is_variable_frame_v<T> ) {
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
            template<typename T = Derived, size_t N>
            Result<span<std::byte, N> > serialize() {
                // * If the frame has a checksum and it's dirty, update it
                if constexpr (has_checksum_v<T> ) {
                    if (this->is_checksum_dirty()) {
                        auto res = this->update_checksum();
                        if (!res) {
                            return Result<span<std::byte, N> >::error(res.error(),
                                "CoreInterface::serialize");
                        }
                    }
                }
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
                // * validate the input data
                auto result = this->validate(data);
                if (!result) {
                    return Result<void>::error(result.error(), "CoreInterface::deserialize");
                }
                // * call derived class to perform deserialization
                result = derived().impl_deserialize(data);
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
                return dump_frame<Derived>(frame_storage_);
            }
            /**
             * @brief Get the size of the frame in bytes.
             * @note If the frame has a fixed size, this returns that size. If variable, it calls derived() to compute the size.
             * @return Result<std::size_t> The size of the frame in bytes.
             */
            template<typename T = Derived>
            Result<std::size_t> size() const {
                if  constexpr (!is_variable_frame_v<T> ) {
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
            template<typename T = Derived>
            std::enable_if_t<!is_variable_frame_v<T>, Result<void> >
            set_type(Type type) {
                auto res = this->validate_type(to_byte(type));
                if (!res) {
                    return res.error();
                }
                return derived().impl_set_type(type);
            }
            /**
             * @brief Set the Type byte of the frame.
             * @return Result<Type> The Type byte of the frame.
             * @note This calls derived().get_type() for frame-specific type retrieval.
             */
            template<typename T = Derived>
            std::enable_if_t<is_variable_frame_v<T>, Result<void> >
            set_type(std::byte type) {
                auto res = this->validate_type(type);
                if (!res) {
                    return res.error();
                }
                return derived().impl_set_type(type);
            }

    };
}
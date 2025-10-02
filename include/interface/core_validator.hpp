/**
 * @file core_validator.hpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief Core validation utilities for frame types.
 * @version 0.1
 * @date 2025-10-02
 *
 * @copyright Copyright (c) 2025
 */


#pragma once

#include "../result.hpp"
#include "../frame_traits.hpp"
#include "../protocol.hpp"

#include <boost/core/span.hpp>
#include <cstddef>

using namespace boost;

namespace USBCANBridge {
    /**
     * @brief Core validation interface for all frames
     * This class provides validation methods for the methods provided by the CoreInterface.
     * @tparam Derived
     * @note This class uses CRTP to call derived class implementations for specific validations when needed.
     */
    template<typename Derived>
    class CoreValidator {
        protected:
            // * CRTP helper to access derived class methods
            Derived& derived() {
                return static_cast<Derived&>(*this);
            }
            // * CRTP helper to access derived class methods (const version)
            const Derived& derived() const {
                return static_cast<const Derived&>(*this);
            }
        private:
            /**
             * @brief Internal helper to perform type validation logic.
             * @note For the variable frame, this checks the individual components of the type byte but does not check dlc against actual payload size. This is because the actual payload size may not be known at this point and is validated separately when the payload is set or modified.
             * @param type The Type to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             */
            static Result<bool> validate_type_impl(std::byte type) {
                if constexpr (is_fixed_frame_v<Derived> ) {
                    return (from_byte<Type>(type) == Type::DATA_FIXED) ?
                           Result<bool>::success(true) :
                           Result<bool>::error(Status::WBAD_TYPE);
                }
                if constexpr (is_variable_frame_v<Derived> ) {
                    Type type_base = extract_type_base(type);
                    FrameType frame_type = extract_frame_type(type);
                    FrameFormat frame_fmt = extract_frame_format(type);
                    std::size_t dlc = extract_dlc(type);
                    bool valid = (type_base == Type::DATA_VARIABLE) &&
                        (frame_type == FrameType::STD_VARIABLE ||
                        frame_type == FrameType::EXT_VARIABLE) &&
                        (frame_fmt == FrameFormat::REMOTE_VARIABLE ||
                        frame_fmt == FrameFormat::DATA_VARIABLE) &&
                        (dlc <= Derived::layout::MAX_DATA_SIZE);
                    return valid ? Result<bool>::success(true) :
                           Result<bool>::error(Status::WBAD_TYPE);
                }
                if constexpr (is_config_frame_v<Derived> ) {
                    return (from_byte<Type>(type) == Type::CONF_FIXED ||
                           from_byte<Type>(type) == Type::CONF_VARIABLE) ?
                           Result<bool>::success(true) :
                           Result<bool>::error(Status::WBAD_TYPE);
                }
                return Result<bool>::error(Status::WBAD_TYPE); // Unknown frame type
            }

        public:
            /**
             * @brief Validate the frame.
             * @return Result<Status> Status::SUCCESS if the frame is valid, or an error status if invalid.
             * @note This calls derived().impl_validate() for frame-specific validation.
             */
            Result<void> validate() {
                return derived().impl_validate();
            }
            /**
             * @brief Validate the buffer returned when serializing the frame.
             * This method allows to check if the internal state of the frame is consistent before serialization, so that we can avoid useless checks when we want to retrieve the raw data.
             * @note The check is performed against the deduced frame using the Derived type.
             * @tparam T The frame class type (FixedFrame, VariableFrame, ConfigFrame).
             * @param buffer The buffer to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             */
            template<typename T = Derived, size_t N>
            Result<bool> validate_storage_size(span<const std::byte, N> buffer) const {
                // if T has fixed size we can exploit the fact that it has a checksum
                if constexpr (has_checksum_v<T> ) {
                    if (buffer.size() != derived().layout::FRAME_SIZE) {
                        return Result<bool>::error(Status::WBAD_LENGTH);
                    }
                    return Result<bool>::success(true);
                } else {
                    /** if T has variable size, we need to check:
                     *  - min_size <= buffer.size() <= max_size
                     *  - buffer.size() matches what is expected from its layout (inspected via dlc)
                     */
                    if (buffer.size() < derived().layout::MIN_FRAME_SIZE ||
                        buffer.size() > derived().layout::MAX_FRAME_SIZE ||
                        buffer.size() != derived().layout::frame_size(
                        derived().is_extended(), derived().get_dlc().value()
                        )
                    ) {
                        return Result<bool>::error(Status::WBAD_LENGTH);
                    }
                    return Result<bool>::success(true);
                }
            }


            /**
             * @brief Validate the given Type object before setting it.
             * @param type The Type to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @note This method uses compile-time checks to validate the type based on frame type.
             */
            Result<bool> validate_type(std::byte type) const {
                return validate_type_impl(type);
            }

            // === Static aliases for easier usage ===

            /**
             * @brief Static convenience method to validate type without an instance.
             * @param type The Type to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @note This is a static method that provides the same validation logic without requiring an instance.
             */
            static Result<bool> static_validate_type(std::byte type) {
                return validate_type_impl(type);
            }

    };

}
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

#include "../template/result.hpp"
#include "../template/frame_traits.hpp"
#include "../enums/protocol.hpp"

#include <boost/core/span.hpp>
#include <cstddef>

using namespace boost;

namespace USBCANBridge {
    /**
     * @brief Core validation utilities for all frame types
     * This class provides standalone validation methods that work with any frame type.
     * @tparam Frame The frame type to validate
     */
    template<typename Frame>
    class CoreValidator {
        private:
            const Frame* frame_;

        public:
            explicit CoreValidator(const Frame& frame) : frame_(&frame) {
            }

        private:
            /**
             * @brief Internal helper to perform type validation logic.
             * @note For the variable frame, this checks the individual components of the type byte but does not check dlc against actual payload size. This is because the actual payload size may not be known at this point and is validated separately when the payload is set or modified.
             * @param type The Type to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             */
            template<typename T = Frame>
            static Result<bool> validate_type_impl(std::byte type) {
                if constexpr (is_fixed_frame_v<T>) {
                    return (from_byte<Type>(type) == Type::DATA_FIXED) ?
                           Result<bool>::success(true) :
                           Result<bool>::error(Status::WBAD_TYPE);
                }
                if constexpr (is_variable_frame_v<T>) {
                    Type type_base = extract_type_base(type);                   
                    FrameFormat frame_fmt = extract_frame_format(type);
                    std::size_t dlc = extract_dlc(type);
                    bool valid = (type_base == Type::DATA_VARIABLE) &&
                        validate_frame_type_impl(type).value() &&
                        (frame_fmt == FrameFormat::REMOTE_VARIABLE ||
                        frame_fmt == FrameFormat::DATA_VARIABLE) &&
                        (dlc <= T::layout::MAX_DATA_SIZE);
                    return valid ? Result<bool>::success(true) :
                           Result<bool>::error(Status::WBAD_TYPE);
                }
                if constexpr (is_config_frame_v<T>) {
                    return (from_byte<Type>(type) == Type::CONF_FIXED ||
                           from_byte<Type>(type) == Type::CONF_VARIABLE) ?
                           Result<bool>::success(true) :
                           Result<bool>::error(Status::WBAD_TYPE);
                }
                return Result<bool>::error(Status::WBAD_TYPE); // Unknown frame type
            }

            /**
             * @brief Internal helper to perform frame type validation logic.
             * @param frame_type The FrameType to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             */
            template<typename T = Frame>
            static Result<bool> validate_frame_type_impl(std::byte frame_type) {
                if constexpr (is_fixed_frame_v<T>) {
                    FrameType ft = extract_frame_type(frame_type);
                    return (ft == FrameType::STD_FIXED || ft == FrameType::EXT_FIXED) ?
                           Result<bool>::success(true) :
                           Result<bool>::error(Status::WBAD_FRAME_TYPE);
                }
                if constexpr (is_variable_frame_v<T>) {
                    FrameType ft = extract_frame_type(frame_type);
                    return (ft == FrameType::STD_VARIABLE || ft == FrameType::EXT_VARIABLE) ?
                           Result<bool>::success(true) :
                           Result<bool>::error(Status::WBAD_FRAME_TYPE);
                }
                if constexpr (is_config_frame_v<T>) {
                    FrameType ft = extract_frame_type(frame_type);
                    return (ft == FrameType::STD_FIXED || ft == FrameType::EXT_FIXED) ?
                           Result<bool>::success(true) :
                           Result<bool>::error(Status::WBAD_FRAME_TYPE);
                }
                return Result<bool>::error(Status::WBAD_FRAME_TYPE); // Unknown frame type
            }

        public:
            /**
             * @brief Validate the frame.
             * @param data The buffer to validate.
             * @return Result<bool> Status::SUCCESS if the frame is valid, or an error status if invalid.
             * @note This calls frame_->impl_validate() for frame-specific validation.
             */
            Result<bool> validate(span<const std::byte> data) const {
                return frame_->impl_validate(data);
            }
            /**
             * @brief Validate the buffer returned when serializing the frame.
             * This method allows to check if the internal state of the frame is consistent before serialization, so that we can avoid useless checks when we want to retrieve the raw data.
             * @note The check is performed against the deduced frame using the Frame type.
             * @tparam T The frame class type (FixedFrame, VariableFrame, ConfigFrame).
             * @param buffer The buffer to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             */
            template<typename T = Frame, size_t N>
            Result<bool> validate_storage_size(span<const std::byte, N> buffer) const {
                using traits = frame_traits_t<T>;
                // if T has fixed size we can exploit the fact that it has a checksum
                if constexpr (has_checksum_v<T>) {
                    if (buffer.size() != traits::FRAME_SIZE) {
                        return Result<bool>::error(Status::WBAD_LENGTH);
                    }
                    return Result<bool>::success(true);
                } else {
                    /** if T has variable size, we need to check:
                     *  - min_size <= buffer.size() <= max_size
                     *  - buffer.size() matches what is expected from its layout (inspected via dlc)
                     */
                    if (buffer.size() < traits::MIN_FRAME_SIZE ||
                        buffer.size() > traits::MAX_FRAME_SIZE ||
                        buffer.size() != traits::Layout::frame_size(
                        frame_->is_extended(), frame_->get_dlc().value()
                        )
                    ) {
                        return Result<bool>::error(Status::WBAD_LENGTH);
                    }
                    return Result<bool>::success(true);
                }
            }

            /**
             * @brief Validate the frame's `START` byte
             * This method checks if the `START` byte is valid.
             */
            Result<bool> validate_start_byte(std::byte start_byte) const {
                if (start_byte != to_byte(Constants::START_BYTE)) {
                    return Result<bool>::error(Status::WBAD_START);
                }
                return Result<bool>::success(true);
            }

            /**
             * @brief Validate the given Type object before setting it.
             * @param type The Type to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @note This method uses compile-time checks to validate the type based on frame type.
             */
            Result<bool> validate_type_byte(std::byte type) const {
                return validate_type_impl<Frame>(type);
            }

            // === Static aliases for easier usage ===

            /**
             * @brief Static convenience method to validate type without an instance.
             * @param type The Type to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @note This is a static method that provides the same validation logic without requiring an instance.
             */
            static Result<bool> static_validate_type(std::byte type) {
                return validate_type_impl<Frame>(type);
            }

            /**
             * @brief Validate the given FrameType object before setting it.
             * @param frame_type The FrameType to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @note This method uses compile-time checks to validate the frame type based on frame type.
             */
            Result<bool> validate_frame_type(std::byte frame_type) const {
                return validate_frame_type_impl<Frame>(frame_type);
            }

    };

}
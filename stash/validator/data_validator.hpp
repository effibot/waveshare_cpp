/**
 * @file data_validator.hpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief Data validation utilities for frame types.
 * @version 0.1
 * @date 2025-10-02
 *
 * @copyright Copyright (c) 2025
 */

#pragma once
#include "../template/frame_traits.hpp"
#include "../enums/protocol.hpp"

namespace USBCANBridge {
    /**
     * @brief Data frame validation utilities
     * This class provides standalone validation methods for data frame types.
     * @tparam Frame The frame type to validate
     */
    template<typename Frame>
    class DataValidator {
        private:
            const Frame* frame_;

        public:
            explicit DataValidator(const Frame& frame) : frame_(&frame) {
            }

        private:
            /**
             * @brief Internal helper to perform CAN ID validation logic.
             * If the frame supports extended IDs, all 29 bits are valid.
             * If the frame supports only standard IDs, only the upper 11 bits are valid.
             * @param id The CAN ID to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             */
            static Result<bool> validate_can_id_internal(bool is_extended, uint32_t id) {
                if (is_extended) {
                    return (id <= 0x1FFFFFFF) ?
                           Result<bool>::success(true) :
                           Result<bool>::error(Status::WBAD_ID);
                } else {
                    return (id <= 0x7FF) ?
                           Result<bool>::success(true) :
                           Result<bool>::error(Status::WBAD_ID);
                }
            }
            /**
             * @brief Internal helper to perform data length validation logic.
             * @param length The length of the data to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             */
            static Result<bool> validate_data_length_internal(std::size_t length) {
                return (length == MAX_DATA_LENGTH) ?
                       Result<bool>::success(true) :
                       Result<bool>::error(Status::WBAD_LENGTH);
            }
            /**
             * @brief Internal Helper to get the header byte from the frame storage.
             *
             * @return uint8_t The header byte.
             */
            template<typename T = Frame>
            std::enable_if_t<is_fixed_frame_v<T>, Result<uint8_t> >
            get_header_byte() const {
                return frame_->impl_get_header_byte();
            }

        public:
            /**
             * @brief Validate the [Header] byte of a FixedFrame.
             * @note The header byte is initialized to Constants::HEADER (`0x55`) and should not change.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             */
            template<typename T = Frame>
            std::enable_if_t<is_fixed_frame_v<T>, Result<bool> >
            validate_header_byte() const {
                if (get_header_byte() == Constants::HEADER) {
                    return Result<bool>::success(true);
                }
                return Result<bool>::error(Status::WBAD_HEADER);
            }

            /**
             * @brief Validate the format byte before setting it.
             *
             * @param format The format byte to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             */
            static Result<bool> validate_format(Format format) {
                if (format == Format::DATA_VARIABLE ||
                    format == Format::REMOTE_VARIABLE) {
                    return Result<bool>::success(true);
                }
                return Result<bool>::error(Status::WBAD_FORMAT);
            }

            /**
             * @brief Validate the CAN ID.
             * @param id The CAN ID to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @note This uses the frame's is_extended() method to determine validation rules.
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, Result<bool> >
            validate_can_id(uint32_t id) const {
                return validate_can_id_internal(frame_->is_extended().value(), id);
            }
            /**
             * @brief Validate the data length before setting the data.
             * @param length The length of the data to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @note This validates against the maximum data length.
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, Result<bool> >
            validate_data_length(std::size_t length) const {
                return validate_data_length_internal(length);
            }


    };

}
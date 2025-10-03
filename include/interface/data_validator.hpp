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
#include "../template/result.hpp"
#include "../template/frame_traits.hpp"
#include "../enums/protocol.hpp"

namespace USBCANBridge {
    /**
     * @brief Data frame validation
     * This class provides validation methods for the methods provided by the DataInterface.
     * @tparam Derived
     * @note This class uses CRTP to call derived class implementations for specific validations when needed.
     */
    template<typename Derived>
    class DataValidator {
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
            template<typename T = Derived>
            std::enable_if_t<is_fixed_frame_v<T>, Result<uint8_t> >
            get_header_byte() const {
                return derived().impl_get_header_byte();
            }

        public:
            /**
             * @brief Validate the [Header] byte of a FixedFrame.
             * @note The header byte is initialized to Constants::HEADER (`0x55`) and should not change.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             */
            template<typename T = Derived>
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
            static Result<bool> validate_format(FrameFormat format) {
                if (format == FrameFormat::DATA_VARIABLE ||
                    format == FrameFormat::REMOTE_VARIABLE) {
                    return Result<bool>::success(true);
                }
                return Result<bool>::error(Status::WBAD_FORMAT);
            }

            /**
             * @brief Validate the CAN ID.
             * @param id The CAN ID to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @note This calls derived().impl_validate_can_id() for frame-specific validation.
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<bool> >
            validate_can_id(uint32_t id) const {
                return static_cast<const T*>(this)->validate_can_id_internal(
                    derived().is_extended().value(), id);
            }
            /**
             * @brief Validate the data length before setting the data.
             * @param length The length of the data to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @note This calls derived().impl_validate_data_length() for frame-specific validation.
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<bool> >
            validate_data_length(std::size_t length) const {
                return static_cast<const T*>(this)->validate_data_length_internal(length);
            }


    };

}
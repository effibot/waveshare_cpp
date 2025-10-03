/**
 * @file config_frame.cpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief Implementation of ConfigFrame methods.
 * @version 0.1
 * @date 2025-10-03
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "../include/frame/config_frame.hpp"


namespace USBCANBridge {

    namespace CONFIG {
        constexpr size_t _START_ = ConfigFrame::layout::START_OFFSET;
        constexpr size_t _HEADER_ = ConfigFrame::layout::HEADER_OFFSET;
        constexpr size_t _TYPE_ = ConfigFrame::layout::TYPE_OFFSET;
        constexpr size_t _BAUD_ = ConfigFrame::layout::BAUD_OFFSET;
        constexpr size_t _MODE_ = ConfigFrame::layout::MODE_OFFSET;
        constexpr size_t _FILTER_ = ConfigFrame::layout::FILTER_OFFSET;
        constexpr size_t _MASK_ = ConfigFrame::layout::MASK_OFFSET;
        constexpr size_t _RESERVED_ = ConfigFrame::layout::RESERVED_OFFSET;
        constexpr size_t _RESERVED_SIZE_ = ConfigFrame::layout::RESERVED_SIZE;
        constexpr size_t _CHECKSUM_ = ConfigFrame::layout::CHECKSUM_OFFSET;
    }

    Result<bool> ConfigFrame::impl_validate(span<const std::byte> data) const {
        // * Check buffer size
        auto valid = this->validator_.validate_storage_size(data);
        if (!valid) {
            return valid;
        }
        // * Check the [START] and the [HEADER] bytes
        valid = this->validator_.validate_start_byte(data[CONFIG::_START_]);
        if (!valid) {
            return valid;
        }
        valid =
            this->config_validator_.validate_header_byte(data[CONFIG::_HEADER_]);
        if (!valid) {
            return valid;
        }
        // * Check the [TYPE] byte
        valid = this->validator_.validate_type_byte(data[CONFIG::_TYPE_]);
        if (!valid) {
            return valid;
        }
        // * Check the [BAUD] byte
        valid = this->config_validator_.validateBaudRate(from_byte<CANBaud>(data[CONFIG::_BAUD_]));
        if (!valid) {
            return valid;
        }
        // * Check the [MODE] byte
        valid = this->config_validator_.validateCanMode(from_byte<CANMode>(data[CONFIG::_MODE_]));
        if (!valid) {
            return valid;
        }
        // * Check the [FILTER] byte
        Result<uint32_t> filter = this->get_filter();
        valid = this->config_validator_.validateFilter(filter.value());
        if (!valid) {
            return valid;
        }
        // * Check the [MASK] byte
        Result<uint32_t> mask = this->get_mask();
        valid = this->config_validator_.validateMask(mask.value());
        if (!valid) {
            return valid;
        }
        // * Check the [RESERVED] byte
        span<const std::byte> reserved_bytes = data.subspan(CONFIG::_RESERVED_,
            CONFIG::_RESERVED_SIZE_);
        valid = this->config_validator_.validateReservedBytes(reserved_bytes);
        if (!valid) {
            return valid;
        }
        // * Check the [CHECKSUM] byte
        valid = this->checksum_interface_.verify_checksum(data);
        if (!valid) {
            return valid;
        }
        // All checks passed
        return Result<bool>::success(true);
    }


} // namespace USBCANBridge
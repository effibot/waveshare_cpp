/**
 * @file config_validator.hpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief Configuration validation utilities for frame types.
 * @version 0.1
 * @date 2025-10-02
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include "../template/result.hpp"
#include "../template/frame_traits.hpp"
#include "../enums/protocol.hpp"

namespace USBCANBridge {
    /**
     * @brief Configuration validation utilities for frame types.
     * This class provides standalone validation methods for configuration frame types.
     * @tparam Frame The frame type to validate.
     */
    template<typename Frame>
    class ConfigValidator {
        static_assert(is_config_frame_v<Frame>,
            "ConfigValidator can only be used with config frame types");
        private:
            const Frame* frame_;

        public:
            explicit ConfigValidator(const Frame& frame) : frame_(&frame) {
            }

        public:

            /**
             * @brief Validate the Header byte of the config frame.
             * @param header The Header byte to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             */
            Result<bool> validate_header_byte(std::byte header) const {
                if (header == to_byte(Constants::HEADER)) {
                    return Result<bool>::success(true);
                }
                return Result<bool>::error(Status::WBAD_HEADER);
            }
            /**
             * @brief Validate the given baud rate against the supported rates.
             * @param rate The CAN baud rate to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @see CANBaud in protocol.hpp for supported rates.
             */
            Result<bool> validateBaudRate(CANBaud rate) const {
                switch (rate) {
                case CANBaud::BAUD_10K:
                case CANBaud::BAUD_20K:
                case CANBaud::BAUD_50K:
                case CANBaud::BAUD_100K:
                case CANBaud::BAUD_125K:
                case CANBaud::BAUD_250K:
                case CANBaud::BAUD_500K:
                case CANBaud::BAUD_800K:
                case CANBaud::BAUD_1M:
                    return Result<bool>::success(true);
                default:
                    return Result<bool>::error(Status::WBAD_CAN_BAUD);
                }
            }
            /**
             * @brief Validate the given CAN mode against the supported modes.
             * @param mode The CAN mode to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @see CANMode in protocol.hpp for supported modes.
             */
            Result<bool> validateCanMode(CANMode mode) const {
                switch (mode) {
                case CANMode::NORMAL:
                case CANMode::SILENT:
                case CANMode::LOOPBACK:
                case CANMode::LOOPBACK_SILENT:
                    return Result<bool>::success(true);
                default:
                    return Result<bool>::error(Status::WBAD_CAN_MODE);
                }
            }
            /**
             * @brief Validate the given acceptance filter value.
             * The filter mask has to match the selected CAN ID type (standard or extended).
             * @param filter The acceptance filter to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @note The acceptance filter is a 32-bit mask. Any value inside the CAN2.0A ([`0x0`-`0x7FF`]) or CAN2.0B ([`0x0`-`0x1FFFFFFF`]) range is considered valid.
             */
            Result<bool> validateFilter(uint32_t filter) const {
                auto frame_type = frame_->get_CAN_version();
                if (!frame_type) {
                    return Result<bool>::error(frame_type.error());
                }

                // Check frame type and corresponding filter limits
                CANVersion ft = frame_type.value();
                if (ft == CANVersion::STD_VARIABLE || ft == CANVersion::STD_FIXED) {
                    return Result<bool>::success(filter <= MAX_CAN_ID_STD);
                } else if (ft == CANVersion::EXT_VARIABLE || ft == CANVersion::EXT_FIXED) {
                    return Result<bool>::success(filter <= MAX_CAN_ID_EXT);
                }
                return Result<bool>::error(Status::WBAD_FILTER);

            }
            /**
             * @brief Validate the given acceptance mask value.
             * The mask has to match the selected CAN ID type (standard or extended).
             * @param mask The acceptance mask to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @note The acceptance mask is a 32-bit mask. Any value inside the CAN2.0A ([`0x0`-`0x7FF`]) or CAN2.0B ([`0x0`-`0x1FFFFFFF`]) range is considered valid.
             */
            Result<bool> validateMask(uint32_t mask) const {
                auto frame_type = frame_->get_CAN_version();
                if (!frame_type) {
                    return Result<bool>::error(frame_type.error());
                }
                // Check frame type and corresponding filter limits
                CANVersion ft = frame_type.value();
                if (ft == CANVersion::STD_VARIABLE || ft == CANVersion::STD_FIXED) {
                    return Result<bool>::success(mask <= MAX_CAN_ID_STD);
                } else if (ft == CANVersion::EXT_VARIABLE || ft == CANVersion::EXT_FIXED) {
                    return Result<bool>::success(mask <= MAX_CAN_ID_EXT);
                }
                return Result<bool>::error(Status::WBAD_MASK);
            }

            /**
             * @brief Validate the given RTX (retransmission) setting.
             * @param rtx The RTX setting to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @see RTX in protocol.hpp for supported settings.
             */
            Result<bool> validateRTX(RTX rtx) const {
                switch (rtx) {
                case RTX::AUTO:
                case RTX::OFF:
                    return Result<bool>::success(true);
                default:
                    return Result<bool>::error(Status::WBAD_RTX);
                }
            }


            /**
             * @brief Validate the RESERVED byte of the config frame.
             * @param reserved The RESERVED byte to validate as span.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             */
            Result<bool> validateReservedBytes(span<const std::byte> reserved) const {
                for (const auto& byte : reserved) {
                    if (byte != to_byte(Constants::RESERVED)) {
                        return Result<bool>::error(Status::WBAD_RESERVED);
                    }
                }
                return Result<bool>::success(true);
            }

    };
}
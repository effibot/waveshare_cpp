/**
 * @file waveshare_exception.hpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief Exception hierarchy for Waveshare USB-CAN-A library
 * @version 0.1
 * @date 2025-10-14
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include <stdexcept>
#include <string>
#include "../enums/error.hpp"

namespace waveshare {

    /**
     * @class WaveshareException
     * @brief Base exception class for all Waveshare-related errors
     *
     * This exception stores the original Status code for programmatic error handling
     * while providing a descriptive error message via what().
     */
    class WaveshareException : public std::runtime_error {
        protected:
            Status status_;     ///< Original error status code
            std::string context_; ///< Operation context (function name, etc.)

        public:
            /**
             * @brief Construct exception with status code and context
             * @param status The error status code
             * @param context Description of where the error occurred
             */
            WaveshareException(Status status, const std::string& context)
                : std::runtime_error(format_message(status, context)),
                status_(status),
                context_(context) {}

            /**
             * @brief Get the status code
             * @return Status code associated with this exception
             */
            Status status() const noexcept { return status_; }

            /**
             * @brief Get the operation context
             * @return Context string describing where error occurred
             */
            const std::string& context() const noexcept { return context_; }

        private:
            /**
             * @brief Format error message from status and context
             * @param status Error status code
             * @param context Operation context
             * @return Formatted error message
             */
            static std::string format_message(Status status, const std::string& context) {
                USBCANErrorCategory category;
                return "[" + category.message(static_cast<int>(status)) + "] in " + context;
            }
    };

    // === Derived Exception Classes ===

    /**
     * @class ProtocolException
     * @brief Exception for protocol/frame validation errors
     *
     * Used for errors during frame parsing, validation, and protocol violations.
     * Corresponds to WBAD_* status codes.
     */
    class ProtocolException : public WaveshareException {
        public:
            using WaveshareException::WaveshareException;
    };

    /**
     * @class DeviceException
     * @brief Exception for device I/O and configuration errors
     *
     * Used for errors related to USB device operations, serial I/O failures,
     * and device state issues. Corresponds to DNOT_*, DREAD_*, DWRITE_* status codes.
     */
    class DeviceException : public WaveshareException {
        public:
            using WaveshareException::WaveshareException;
    };

    /**
     * @class TimeoutException
     * @brief Exception for timeout errors
     *
     * Used when operations exceed their timeout period.
     * Corresponds to WTIMEOUT status code.
     */
    class TimeoutException : public WaveshareException {
        public:
            using WaveshareException::WaveshareException;
    };

    /**
     * @class CANException
     * @brief Exception for CAN bus protocol errors
     *
     * Used for CANopen-specific errors (SDO, PDO, NMT).
     * Corresponds to CAN_* status codes.
     */
    class CANException : public WaveshareException {
        public:
            using WaveshareException::WaveshareException;
    };

    // === Exception Factory Helpers ===

    /**
     * @brief Throw appropriate exception based on status code
     * @param status The error status code
     * @param context Description of where the error occurred
     * @throws ProtocolException for WBAD_* codes
     * @throws DeviceException for DNOT_*, DREAD_*, DWRITE_* codes
     * @throws TimeoutException for WTIMEOUT
     * @throws CANException for CAN_* codes
     * @throws WaveshareException for other codes
     */
    inline void throw_error(Status status, const std::string& context) {
        // Determine exception type based on status code
        switch (status) {
        // Protocol/validation errors (WBAD_*)
        case Status::WBAD_START:
        case Status::WBAD_HEADER:
        case Status::WBAD_TYPE:
        case Status::WBAD_FRAME_TYPE:
        case Status::WBAD_LENGTH:
        case Status::WBAD_ID:
        case Status::WBAD_DATA:
        case Status::WBAD_DLC:
        case Status::WBAD_FORMAT:
        case Status::WBAD_RESERVED:
        case Status::WBAD_CHECKSUM:
        case Status::WBAD_DATA_INDEX:
        case Status::WBAD_CAN_MODE:
        case Status::WBAD_CAN_BAUD:
        case Status::WBAD_FILTER:
        case Status::WBAD_MASK:
        case Status::WBAD_RTX:
            throw ProtocolException(status, context);

        // Device errors (DNOT_*, DREAD_*, DWRITE_*)
        case Status::DNOT_FOUND:
        case Status::DNOT_OPEN:
        case Status::DALREADY_OPEN:
        case Status::DREAD_ERROR:
        case Status::DWRITE_ERROR:
        case Status::DCONFIG_ERROR:
            throw DeviceException(status, context);

        // Timeout error
        case Status::WTIMEOUT:
            throw TimeoutException(status, context);

        // CAN protocol errors
        case Status::CAN_SDO_TIMEOUT:
        case Status::CAN_SDO_ABORT:
        case Status::CAN_PDO_ERROR:
        case Status::CAN_NMT_ERROR:
            throw CANException(status, context);

        // Unknown or generic error
        default:
            throw WaveshareException(status, context);
        }
    }

    /**
     * @brief Throw if status indicates an error (not SUCCESS)
     * @param status The status code to check
     * @param context Description of where the error occurred
     */
    inline void throw_if_error(Status status, const std::string& context) {
        if (status != Status::SUCCESS) {
            throw_error(status, context);
        }
    }

} // namespace waveshare

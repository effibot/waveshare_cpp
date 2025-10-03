/**
 * @file error.hpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief Error codes for USBCANBridge to be used as template parameter.
 * @version 0.1
 * @date 2025-09-11
 */

#pragma once
#include <string>
#include <system_error>
#include <type_traits>

namespace USBCANBridge {

/**
 * @enum Status
 * @brief Enumeration of error codes for USBCANBridge operations.
 * These codes can be converted to std::error_code for integration with
 * standard error handling mechanisms.
 * @note SUCCESS (0) indicates no error.
 * Other values indicate specific error conditions.
 * If starts with 'W' it is a warning.
 * If starts with 'D' it is a device-related error.
 * If starts with 'CAN' it is a CAN bus related error.
 * @see std::error_code
 */
    enum class Status : int {
        SUCCESS = 0,    /**< No error */
        WBAD_START = 1, /**< Bad start byte */
        WBAD_HEADER = 2, /**< Bad header byte */
        WBAD_TYPE = 3,  /**< Bad message type */
        WBAD_FRAME_TYPE = 4, /**< Bad frame type */
        WBAD_LENGTH = 5, /**< Bad message length */
        WBAD_ID = 6,    /**< Bad CAN ID */
        WBAD_DATA = 7,  /**< Bad data */
        WBAD_DLC = 8,   /**< Bad DLC */
        WBAD_FORMAT = 9, /**< Bad format */
        WBAD_RESERVED = 10, /**< Bad reserved byte */
        WBAD_CHECKSUM = 11, /**< Bad checksum */
        WBAD_DATA_INDEX = 12, /**< Bad data index */
        WBAD_CAN_MODE = 13, /**< Bad CAN mode */
        WBAD_CAN_BAUD = 14, /**< Bad CAN baud rate */
        WBAD_FILTER = 15, /**< Bad ID filter */
        WBAD_MASK = 16, /**< Bad ID mask */
        WBAD_RTX = 17, /**< Bad auto retransmission setting */
        WTIMEOUT = 18,  /**< Timeout */
        DNOT_FOUND = 19, /**< Device not found */
        DNOT_OPEN = 20, /**< Device not open */
        DALREADY_OPEN = 21, /**< Device already open */
        DREAD_ERROR = 22, /**< Device read error */
        DWRITE_ERROR = 23, /**< Device write error */
        DCONFIG_ERROR = 24, /**< Device configuration error */
        CAN_SDO_TIMEOUT = 25, /**< CAN SDO timeout */
        CAN_SDO_ABORT = 26, /**< CAN SDO abort */
        CAN_PDO_ERROR = 27, /**< CAN PDO error */
        CAN_NMT_ERROR = 28, /**< CAN NMT error */
        UNKNOWN = 255   /**< Unknown error */
    };

/**
 * @class USBCANErrorCategory
 * @brief Custom error category for USBCANBridge errors.
 */
    class USBCANErrorCategory : public std::error_category {
        public:
            const char*name() const noexcept override {
                return "USBCANBridge::Status";
            }

            std::string message(int ev) const override {
                switch (static_cast<Status>(ev)) {
                case Status::SUCCESS:
                    return "Success";
                case Status::WBAD_START:
                    return "Bad start byte";
                case Status::WBAD_HEADER:
                    return "Bad header byte";
                case Status::WBAD_TYPE:
                    return "Bad message type";
                case Status::WBAD_FRAME_TYPE:
                    return "Bad frame type";
                case Status::WBAD_LENGTH:
                    return "Bad message length";
                case Status::WBAD_ID:
                    return "Bad CAN ID";
                case Status::WBAD_DATA:
                    return "Bad data";
                case Status::WBAD_DLC:
                    return "Bad DLC";
                case Status::WBAD_FORMAT:
                    return "Bad format";
                case Status::WBAD_RESERVED:
                    return "Bad reserved byte";
                case Status::WBAD_CHECKSUM:
                    return "Bad checksum";
                case Status::WBAD_DATA_INDEX:
                    return "Bad data index";
                case Status::WBAD_CAN_MODE:
                    return "Bad CAN mode";
                case Status::WBAD_FILTER:
                    return "Bad ID filter";
                case Status::WBAD_MASK:
                    return "Bad ID mask";
                case Status::WBAD_CAN_BAUD:
                    return "Bad CAN baud rate";
                case Status::WTIMEOUT:
                    return "Timeout";
                case Status::DNOT_FOUND:
                    return "Device not found";
                case Status::DNOT_OPEN:
                    return "Device not open";
                case Status::DALREADY_OPEN:
                    return "Device already open";
                case Status::DREAD_ERROR:
                    return "Device read error";
                case Status::DWRITE_ERROR:
                    return "Device write error";
                case Status::DCONFIG_ERROR:
                    return "Device configuration error";
                case Status::CAN_SDO_TIMEOUT:
                    return "CAN SDO timeout";
                case Status::CAN_SDO_ABORT:
                    return "CAN SDO abort";
                case Status::CAN_PDO_ERROR:
                    return "CAN PDO error";
                case Status::CAN_NMT_ERROR:
                    return "CAN NMT error";
                case Status::UNKNOWN:
                    return "Unknown error";
                default:
                    return "Unrecognized error";
                }
            }
    };

// Get the error category instance
    inline const std::error_category &usbcan_category() {
        static USBCANErrorCategory instance;
        return instance;
    }

// Make error_code from Status
    inline std::error_code make_error_code(Status e) {
        return {static_cast<int>(e), usbcan_category()};
    }

}; // namespace USBCANBridge

// Register the enum for use with std::error_code
namespace std {
    template<> struct is_error_code_enum<USBCANBridge::Status> : true_type {};
}; // namespace std

/**
 * Usage example:
 * #include "ros2_waveshare/error.hpp"
 *
 * std::error_code ec = USBCANBridge::Status::DNOT_FOUND;
 * if (ec) {
 *     std::cerr << "Error: " << ec.message() << std::endl;
 * }
 */

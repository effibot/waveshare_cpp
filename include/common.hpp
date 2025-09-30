/**
 * @file common.hpp
 * @brief Common utilities and definitions for USB-CAN adapter frames.
 *
 * This file provides shared enums, constants, and helper functions used across
 * different frame implementations for USB-CAN bridge communication. It defines
 * the protocol constants, frame types, baud rates, and communication modes
 * supported by the USB-CAN adapter.
 *
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @version 1.0
 * @date 2025-09-11
 * @copyright Copyright (c) 2025
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <sstream>
#include <iomanip>
#include <string>
#include <array>

#include "span_compat.hpp"
#include "frame_traits.hpp"

/**
 * @namespace USBCANBridge
 * @brief Namespace containing all USB-CAN bridge related functionality.
 *
 * This namespace encapsulates all classes, enums, and functions related to
 * USB-CAN adapter communication, providing a clean separation from other
 * system components.
 */
namespace USBCANBridge {

/**
 * @brief Converts an enum value to std::byte.
 *
 * This template function safely converts any enum type to std::byte by first
 * casting to the underlying type and then to std::byte. It's useful for
 * serializing enum values into byte arrays for USB-CAN frame construction.
 *
 * @tparam EnumType The enum type to convert (must be an enum class)
 * @param value The enum value to convert
 * @return std::byte representation of the enum value
 *
 * @example
 * @code
 * auto start_byte = to_byte(Constants::START_BYTE);
 * auto type_byte = to_byte(Type::DATA_FIXED);
 * @endcode
 */
    template<typename EnumType> constexpr std::byte to_byte(EnumType value) {
        return static_cast<std::byte>(
            static_cast<std::underlying_type_t<EnumType> >(value));
    }

/**
 * @brief Converts an enum value to std::size_t.
 *
 * This template function safely converts any enum type to std::size_t by first
 * casting to the underlying type and then to std::size_t.
 * It's useful for indexing and size calculations when working with enum values.
 *
 * @tparam EnumType The enum type to convert (must be an enum class)
 * @param value The enum value to convert
 * @return std::size_t representation of the enum value
 * @example
 * @code
 * auto size = to_size_t(Constants::START_BYTE);
 * @endcode
 */

    template<typename EnumType> constexpr std::size_t to_size_t(EnumType value) {
        return static_cast<std::size_t>(
            static_cast<std::underlying_type_t<EnumType> >(value));
    }

/**
 * @brief Converts a std::byte to an enum value.
 *
 * This template function safely converts std::byte back to an enum type by
 * first casting to the underlying type and then to the enum. It's useful for
 * deserializing byte arrays back into meaningful enum values when parsing
 * received USB-CAN frames.
 *
 * @tparam EnumType The enum type to convert to (must be an enum class)
 * @param value The std::byte value to convert
 * @return EnumType representation of the byte value
 *
 * @warning Ensure the byte value corresponds to a valid enum value to avoid
 *          undefined behavior.
 *
 * @example
 * @code
 * std::byte received_byte = std::byte{0xAA};
 * auto constant = from_byte<Constants>(received_byte);
 * if (constant == Constants::START_BYTE) {
 *     // Process start of frame
 * }
 * @endcode
 */
    template<typename EnumType> constexpr EnumType from_byte(std::byte value) {
        return static_cast<EnumType>(
            static_cast<std::underlying_type_t<EnumType> >(value));
    }

/**
 * @brief Converts a std::size_t to an enum value.
 * This template function safely converts std::size_t back to an enum type by
 * first casting to the underlying type and then to the enum. It's useful for
 * deserializing size or index values back into meaningful enum values when
 * parsing received USB-CAN frames.
 * @tparam EnumType The enum type to convert to (must be an enum class)
 * @param value The std::size_t value to convert
 * @return EnumType representation of the size_t value
 * @warning Ensure the size_t value corresponds to a valid enum value to avoid
 *          undefined behavior.
 * @example
 * @code
 * std::size_t index = 2;
 * auto fixed_index = from_size_t<FixedSizeIndex>(index);
 * if (fixed_index == FixedSizeIndex::TYPE) {
 *     // Process type field
 * }
 * @endcode
 */
    template<typename EnumType> constexpr EnumType from_size_t(std::size_t value) {
        return static_cast<EnumType>(
            static_cast<std::underlying_type_t<EnumType> >(value));
    }

/**
 * @brief Protocol constants for USB-CAN frame structure.
 *
 * These constants define the fixed byte values used in the USB-CAN protocol
 * for frame delimiting and structure identification. All frames must use
 * these specific values at designated positions to ensure proper communication.
 */
    enum class Constants : std::uint8_t {
        START_BYTE = 0xAA, ///< Frame start delimiter byte - marks beginning of frame
        MSG_HEADER = 0x55, ///< Message header identifier - follows start byte
        END_BYTE = 0x55, ///< Frame end delimiter byte - marks end of frame
        RESERVED0 = 0x00 ///< Reserved byte value - always set to 0x00
    };

/**
 * @brief Frame type classification for USB-CAN messages.
 *
 * Defines the different types of frames that can be sent through the USB-CAN
 * adapter, distinguishing between data transmission and configuration commands,
 * and between fixed-length and variable-length frame formats.
 */
    enum class Type : std::uint8_t {
        DATA_FIXED = 0x01, ///< Fixed-length data frame (20 bytes total)
        DATA_VAR = 0xC0, ///< Variable-length data frame (up to 8 data bytes)
        CONF_FIXED = 0x02, ///< Fixed-length configuration frame
        CONF_VAR = 0x12 ///< Variable-length configuration frame
    };

/**
 * @brief CAN frame identifier format specification.
 *
 * Specifies whether the CAN frame uses standard (11-bit) or extended (29-bit)
 * identifier format, combined with the frame structure type (fixed or
 * variable). This affects how the CAN ID is interpreted and transmitted on the
 * bus.
 */
    enum class FrameType : std::uint8_t {
        STD_FIXED = 0x01, ///< Standard ID (11-bit) with fixed frame structure
        STD_VAR = 0, ///< Standard ID (11-bit) with variable frame structure
        EXT_FIXED = 0x02, ///< Extended ID (29-bit) with fixed frame structure
        EXT_VAR = 1 ///< Extended ID (29-bit) with variable frame structure
    };

/**
 * @brief CAN frame format type (data vs remote).
 *
 * Distinguishes between data frames (carrying actual payload data) and remote
 * transmission request frames (requesting data from another node). The format
 * also indicates whether using fixed or variable frame structure.
 */
    enum class FrameFmt : std::uint8_t {
        DATA_FIXED = 0x01, ///< Data frame with fixed structure (carries data payload)
        REMOTE_FIXED = 0x02, ///< Remote transmission request with fixed structure
        DATA_VAR = 0,  ///< Data frame with variable structure
        REMOTE_VAR = 1 ///< Remote transmission request with variable structure
    };

/**
 * @brief CAN bus baud rate configuration.
 *
 * Defines the supported baud rates for CAN bus communication. The choice of
 * baud rate affects both communication speed and maximum cable length. Higher
 * speeds provide faster data transfer but limit the maximum distance between
 * nodes.
 *
 * @note Cable length limitations (approximate):
 * - 1 Mbps: up to 25m <<< this is the recommended setting >>>
 * - 500 kbps: up to 100m
 * - 250 kbps: up to 250m
 * - 125 kbps: up to 500m
 * - Lower speeds: up to 1000m+
 */
    enum class CANBaud : std::uint8_t {
        SPEED_1000K = 0x01,
        SPEED_800K = 0x02,
        SPEED_500K = 0x03,
        SPEED_400K = 0x04,
        SPEED_250K = 0x05,
        SPEED_200K = 0x06,
        SPEED_125K = 0x07,
        SPEED_100K = 0x08,
        SPEED_50K = 0x09,
        SPEED_20K = 0x0A,
        SPEED_10K = 0x0B,
        SPEED_5K = 0x0C
    };

/**
 * @brief Serial communication baud rates.
 *
 * Defines the supported baud rates for serial communication between the host
 * computer and the USB-CAN adapter. Higher rates enable faster data transfer
 * but may be limited by cable quality and length.
 *
 * @note Choose the highest rate that provides stable communication for your
 * setup.
 * - 9600 bps: Very stable, compatible with most systems
 * - 115200 bps: Common high-speed rate, good for most applications
 * - 1.2288 Mbps and 2 Mbps: Very high speeds, may require quality cables and
 *   short distances
 */
    enum class SerialBaud : std::uint32_t {
        BAUD_9600 = 9600,
        BAUD_19200 = 19200,
        BAUD_38400 = 38400,
        BAUD_57600 = 57600,
        BAUD_115200 = 115200,
        BAUD_1228800 = 1228800,
        BAUD_2000000 = 2000000
    };

/**
 * @brief CAN controller operating modes.
 *
 * Defines the different operating modes for the CAN controller. These modes
 * are primarily used for testing, debugging, and special operational
 * requirements.
 * @note:
 * - NORMAL: Standard operation for normal bus communication.
 * - LOOPBACK: Used for internal testing without affecting the bus.
 * - SILENT: Listen-only mode, useful for monitoring bus traffic without
 *   transmitting.
 * - LOOPBACK_SILENT: Combination of loopback and silent modes for testing
 *   without bus impact.
 */
    enum class CANMode : std::uint8_t {
        NORMAL = 0x00,
        LOOPBACK = 0x01,
        SILENT = 0x02,
        LOOPBACK_SILENT = 0x03
    };

/**
 * @brief Automatic retransmission control.
 *
 * Controls whether the CAN controller automatically retransmits frames
 * that encounter errors, collisions, or arbitration loss on the bus.
 * Disabling automatic retransmission can be useful for time-critical
 * applications.
 * @note:
 * - AUTO: Standard CAN behavior with automatic retransmission enabled.
 * - OFF: Single-shot mode with automatic retransmission disabled.
 */
    enum class RTX : std::uint8_t { AUTO = 0x00, OFF = 0x01 };

/**
 * @brief Byte position indices for fixed-size USB-CAN frames.
 *
 * Defines the byte positions within a fixed-size (20-byte) USB-CAN frame
 * structure. This enum provides a convenient and type-safe way to access
 * specific bytes in the frame without using magic numbers, improving code
 * readability and maintainability.
 *
 * @note Frame structure (20 bytes total):
 * [START][HEADER][TYPE][FRAME_TYPE][FRAME_FMT][ID0-ID3][DLC][DATA0-DATA7][RESERVED][CHECKSUM]
 */
    enum class FixedSizeIndex : std::size_t {
        START = 0, ///< Start delimiter byte (0xAA)
        HEADER = 1, ///< Header byte (0x55)
        TYPE = 2, ///< Frame type byte (Type enum)
        FRAME_TYPE = 3, ///< CAN frame type byte (FrameType enum)
        FRAME_FMT = 4, ///< Frame format byte (FrameFmt enum)
        ID_0 = 5, ///< CAN ID byte 0 (most significant byte)
        ID_1 = 6, ///< CAN ID byte 1
        ID_2 = 7, ///< CAN ID byte 2
        ID_3 = 8, ///< CAN ID byte 3 (least significant byte)
        DLC = 9,  ///< Data length code byte (0-8)
        DATA_0 = 10, ///< Data byte 0
        DATA_1 = 11, ///< Data byte 1
        DATA_2 = 12, ///< Data byte 2
        DATA_3 = 13, ///< Data byte 3
        DATA_4 = 14, ///< Data byte 4
        DATA_5 = 15, ///< Data byte 5
        DATA_6 = 16, ///< Data byte 6
        DATA_7 = 17, ///< Data byte 7
        RESERVED = 18, ///< Reserved byte (always 0x00)
        CHECKSUM = 19 ///< Checksum byte (frame validation)
    };

/**
 * @brief Byte position indices for the Configuration Command frame.
 *
 * It uses most of the same structure as FixedSizeIndex, but with some
 * differences. See the notes below.
 * @note Frame structure (20 bytes total):
 * [START][HEADER][TYPE][CAN_BAUD][FRAME_TYPE][FILTER_ID1-FILTER_ID4][MASK_ID1-MASK_ID4][CAN_MODE][AUTO_RTX][BACKUP0-BACKUP3][CHECKSUM]
 */
    enum class ConfigCommandIndex : std::size_t {
        START = 0, ///< Start delimiter byte (0xAA)
        HEADER = 1, ///< Header byte (0x55)
        TYPE = 2,  ///< Frame type byte (Type enum)
        CAN_BAUD = 3, ///< CAN baud rate byte (CANBaud enum)
        FRAME_TYPE = 4, ///< CAN frame type byte (FrameType enum)
        FILTER_ID_1 = 5, ///< CAN ID byte 0 (most significant byte)
        FILTER_ID_2 = 6, ///< CAN ID byte 1
        FILTER_ID_3 = 7, ///< CAN ID byte 2
        FILTER_ID_4 = 8, ///< CAN ID byte 3 (least significant byte)
        MASK_ID_1 = 9, ///< CAN ID byte 0 (most significant byte)
        MASK_ID_2 = 10, ///< CAN ID byte 1
        MASK_ID_3 = 11, ///< CAN ID byte 2
        MASK_ID_4 = 12, ///< CAN ID byte 3 (least significant byte)
        CAN_MODE = 13, ///< CAN mode byte (CANMode enum)
        AUTO_RTX = 14, ///< Automatic retransmission control byte (RTX enum)
        BACKUP_0 = 15, ///< Backup byte
        BACKUP_1 = 16, ///< Backup byte
        BACKUP_2 = 17, ///< Backup byte
        BACKUP_3 = 18, ///< Backup byte
        CHECKSUM = 19 ///< Checksum byte (frame validation)
    };

    /**
     * @brief Utility function to get a little-endian byte array from an unsigned integer.
     *
     * This function converts an unsigned integer value into a std::array of std::byte
     * representing the value in little-endian byte order. The size of the resulting array
     * is determined by the template parameter N, which must be between 1 and the size
     * of the integer type T.
     * @param T The unsigned integer type (e.g., uint8_t, uint16_t, uint32_t, uint64_t)
     * @param N The number of bytes to extract (must be <= sizeof(T))
     * @param value The unsigned integer value to convert
     * @return std::array<std::byte, N> The little-endian byte array representation of the value
     * @throws static_assert if T is not an unsigned integer type or if N is out of range
     * @example
     * @code
     * auto bytes = int_to_bytes_le<uint32_t, 4>(0x123); // bytes = {0x23, 0x01, 0x00, 0x00}
     * auto bytes2 = int_to_bytes_le<uint16_t, 2>(0x1234); // bytes2 = {0x34, 0x12}
     * @endcode
     */
    template<typename T, std::size_t N>
    constexpr std::array<std::byte, N> int_to_bytes_le(T value) {
        static_assert(std::is_unsigned<T>::value, "T must be an unsigned integer type");
        static_assert(N > 0 && N <= sizeof(T), "N must be between 1 and sizeof(T)");
        std::array<std::byte, N> bytes = {};
        for (std::size_t i = 0; i < N; ++i) {
            bytes[i] = static_cast<std::byte>(value & 0xFF);
            value >>= 8;
        }
        return bytes;
    }
    /**
     * @brief Utility function to get an unsigned integer from a little-endian byte array.
     * This function converts a std::array of std::byte representing a little-endian byte sequence
     * into an unsigned integer value.
     * @param bytes The little-endian byte array (must be <= sizeof(T))
     * @return T The unsigned integer value represented by the byte array
     * @throws static_assert if T is not an unsigned integer type or if N is out of range
     * @example
     * @code
     * auto value = bytes_to_int_le<uint32_t>(bytes); // value = 0x123
     * @endcode
     */
    template<typename T>
    constexpr T bytes_to_int_le(const std::array<std::byte, sizeof(T)>& bytes) {
        static_assert(std::is_unsigned<T>::value, "T must be an unsigned integer type");
        T value = 0;
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            value |= (static_cast<T>(bytes[i]) & 0xFF) << (8 * i);
        }
        return value;
    }
    /**
     * @brief Utility function to convert a byte array to a hex string.
     * This function converts a std::array of std::byte into a human-readable
     * hexadecimal string representation, with each byte separated by spaces.
     * @param bytes The byte array to convert
     * @return std::string The hexadecimal string representation of the byte array
     * @example
     * @code
     * auto hex_str = bytes_to_hex_string(bytes); // hex_str = "AA 55 01 ..."
     * @endcode
     */
    template<std::size_t N>
    std::string bytes_to_hex_string(const std::array<std::byte, N>& bytes) {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (const auto& byte : bytes) {
            oss << std::setw(2) << static_cast<int>(byte) << " ";
        }
        std::string result = oss.str();
        if (!result.empty()) {
            result.pop_back(); // Remove trailing space
        }
        return result;
    }
    /**
     * @brief Utility function to convert a string of hex values to a byte array.
     * This function converts a string containing hexadecimal byte values (e.g., "AA 55 01")
     * into a std::array of std::byte. The function automatically detect if the input string has spaces or not and handles both cases.
     * @tparam N The number of bytes expected in the output array
     * @param hex_str The hexadecimal string to convert
     * @return std::array<std::byte, N> The converted byte array
     * @throws std::invalid_argument if the input string is invalid
     */
    template<std::size_t N>
    std::array<std::byte, N> hex_string_to_bytes(const std::string& hex_str) {
        std::array<std::byte, N> bytes = {};
        std::istringstream iss(hex_str);
        std::string byte_str;

        for (std::size_t i = 0; i < N; ++i) {
            if (!(iss >> byte_str)) {
                throw std::invalid_argument("Invalid hex string");
            }
            bytes[i] = static_cast<std::byte>(std::stoi(byte_str, nullptr, 16));
        }
        return bytes;
    }

    /**
     * @brief Compute the Type byte for VariableFrame based on FrameFmt, FrameType and DLC.
     *
     * @note The value is computed as follows:
     * - Bits 7-6: ALWAYS `11` for VariableFrame. Use `constants::DATA_VAR`
     * - Bit 5: Frame Type. Use `constants::STD_VAR` or `constants::EXT_VAR`
     * - Bit 4: Frame Format. Use `constants::DATA_VAR` or `constants::REMOTE_VAR`
     * - Bits 3-0: DLC (Data Length Code)
     * @param frame_type Frame type (fixed or variable)
     * @param frame_fmt Frame format (standard or extended)
     * @param dlc Data Length Code (0-8)
     * @return Type The computed Type byte for VariableFrame
     */
    constexpr std::byte compute_type(const FrameType frame_type, const FrameFmt frame_fmt,
        const std::byte dlc) {
        std::uint8_t type = 0xC0; // Start with bits 7-6 set to `11` for VariableFrame
        type |= (to_size_t(frame_type) & 0x01) << 5; // Set bit 5 for Frame Type
        type |= (to_size_t(frame_fmt) & 0x01) << 4; // Set bit 4 for Frame Format
        type |= (static_cast<std::uint8_t>(dlc) & 0x0F); // Set bits 3-0 for DLC
        return static_cast<std::byte>(type);
    }

    /**
     * @brief Utility function to compute checksum for a byte array.
     * The checksum has a meaning only within the context of a ConfigFrame or a FixedFrame.
     * It is computed as the sum (or XOR) of all bytes from the Type to the Reserved byte,
     * The result is taken as the lower 8 bits of the sum.
     * @param data The byte array to compute the checksum for
     * @return std::byte The computed checksum byte
     */
    template<typename T>
    std::byte<
    compute_checksum(const storage_t<T>& data) {
        std::uint8_t sum = 0;
        for (const auto& byte : data) {
            sum += static_cast<std::uint8_t>(byte);
        }
        return static_cast<std::byte>(sum & 0xFF);
    }
} // namespace USBCANBridge
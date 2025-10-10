/**
 * @file protocol.hpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief Protocol definitions and helper functions for USB-CAN bridge.
 * @version 3.0 - State-First Architecture
 * @date 2025-10-09
 *
 * Protocol constants and enum definitions for USB-CAN adapter communication.
 * Compatible with state-first frame architecture (v3.0).
 *
 * @copyright Copyright (c) 2025
 *
 */


#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <string>
#include <sstream>
#include <iomanip>
#include <array>
#include <boost/core/span.hpp>
#include <termios.h>
using namespace boost;

/**
 * @namespace USBCANBridge
 * @brief Namespace containing all USB-CAN bridge related functionality.
 */
namespace USBCANBridge {
    // === Utility Defines ===
    #define MAX_DATA_LENGTH 8
    #define MIN_CAN_ID 0x0
    #define MAX_CAN_ID_EXT 0x1FFFFFFF
    #define MAX_CAN_ID_STD 0x7FF

    // === Frame Byte Constants ===

    /**
     * @brief Frame byte constants for USB-CAN communication.
     *
     * These constants define the special byte values used in the USB-CAN
     * communication protocol. They include the start and end bytes that
     * delimit frames, as well as reserved bytes for future use.
     */
    enum class Constants : std::uint8_t {
        START_BYTE = 0xAA,
        END_BYTE = 0x55,
        HEADER = 0x55,
        RESERVED = 0x00,
    };

    /**
     * @brief Frame type constants for USB-CAN communication.
     *
     * These constants define the different types of frames used in the USB-CAN
     * communication protocol.
     * @note Available frame types are:
     *
     * - DATA_FIXED: Data frame of fixed length (20 bytes total)
     *
     * - DATA_VARIABLE: Variable-length data frame (5 to 13 bytes total)
     *
     * - CONF_FIXED: Set the adapter to accept fixed-length data frames
     *
     * - CONF_VARIABLE: Set the adapter to accept variable-length data frames
     * @attention The CONF_FIXED and CONF_VARIABLE types are used to configure the adapter's behavior and are not used for regular data transmission.Only one configuration frame should be sent to the adapter at startup to set the desired mode.
     */
    enum class Type : std::uint8_t {
        DATA_FIXED = 0x01,
        DATA_VARIABLE = 0xC0,
        CONF_FIXED = 0x02,
        CONF_VARIABLE = 0x12
    };
    // * Define default Type for ConfigFrame
    static constexpr Type DEFAULT_CONF_TYPE = Type::CONF_VARIABLE;

    /**
     * @brief Frame CAN ID Type constants.
     * Defines the CAN ID format (standard or extended) used in the protocol.
     * @note Available CAN ID types are:
     * - STD_FIXED: Standard ID (11-bit) with fixed frame structure
     *
     * - STD_VARIABLE: Standard ID (11-bit) with variable frame structure
     *
     * - EXT_FIXED: Extended ID (29-bit) with fixed frame structure
     *
     * - EXT_VARIABLE: Extended ID (29-bit) with variable frame structure
     */
    enum class CANVersion : std::uint8_t {
        STD_FIXED = 0x01,   // <<< Config Recommended
        STD_VARIABLE = 0,   // <<< Data Recommended
        EXT_FIXED = 0x02,
        EXT_VARIABLE = 1
    };

    // * Define default Frame Type
    static constexpr CANVersion DEFAULT_CAN_VERSION = CANVersion::STD_VARIABLE;

    /**
     * @brief Frame Format constants.
     * Defines the frame format (data or remote) used in the protocol.
     * @note Available frame formats are:
     * - DATA_FIXED: Data frame with fixed structure (carries data payload)
     *
     * - REMOTE_FIXED: Remote transmission request with fixed structure
     *
     * - DATA_VARIABLE: Data frame with variable structure
     *
     * - REMOTE_VARIABLE: Remote transmission request with variable structure
     */
    enum class Format : std::uint8_t {
        DATA_VARIABLE = 0x00,    // Variable length data frame
        DATA_FIXED = 0x01,       // Fixed length data frame
        REMOTE_VARIABLE = 0x01,  // Variable length remote frame
        REMOTE_FIXED = 0x02      // Fixed length remote frame
    };
    // * Define default Frame Format
    static constexpr Format DEFAULT_FORMAT = Format::DATA_VARIABLE;



    // === Config Constants ===
    /**
     * @brief CAN bus baud rate configuration.
     *
     * Defines the supported baud rates for CAN bus communication. The choice of
     * baud rate affects both communication speed and maximum cable length. Higher
     * speeds provide faster data transfer but limit the maximum distance between
     * nodes.
     *
     * @note Cable length limitations (approximate):
     *
     * - 1 Mbps: up to 25m
     *
     * - 500 kbps: up to 100m
     *
     * - 250 kbps: up to 250m
     *
     * - 125 kbps: up to 500m
     *
     * - Lower speeds: up to 1000m+
     */
    enum class CANBaud : std::uint8_t {
        BAUD_1M = 0x01, // <<< Recommended
        BAUD_800K = 0x02,
        BAUD_500K = 0x03,
        BAUD_400K = 0x04,
        BAUD_250K = 0x05,
        BAUD_200K = 0x06,
        BAUD_125K = 0x07,
        BAUD_100K = 0x08,
        BAUD_50K = 0x09,
        BAUD_20K = 0x0A,
        BAUD_10K = 0x0B,
        BAUD_5K = 0x0C
    };
    // * Define default CAN baud rate
    static constexpr CANBaud DEFAULT_CAN_BAUD = CANBaud::BAUD_1M;

    /**
     * @brief CAN controller operating modes.
     * Defines the different operating modes for the CAN controller.
     * Besides NORMAL mode, other modes are useful for testing and diagnostics rather than regular operation.
     * @note Available CAN modes are:
     *
     * - NORMAL: Standard operation for normal bus communication.
     *
     * - LOOPBACK: Used for internal testing without affecting the bus.
     *
     * - SILENT: Listen-only mode, useful for monitoring bus traffic without transmitting.
     *
     * - LOOPBACK_SILENT: Combination of loopback and silent modes for testing
     *   without bus impact.
     */
    enum class CANMode : std::uint8_t {
        NORMAL = 0x00,  // <<< Recommended
        LOOPBACK = 0x01,
        SILENT = 0x02,
        LOOPBACK_SILENT = 0x03
    };
    static constexpr CANMode DEFAULT_CAN_MODE = CANMode::NORMAL;

    /**
     * @brief Automatic retransmission control.
     *
     * Controls whether the CAN controller automatically retransmits frames
     * that encounter errors, collisions, or arbitration loss on the bus.
     * Disabling automatic retransmission can be useful for time-critical
     * applications.
     * @note Available retransmission modes are:
     * - AUTO: Standard CAN behavior with automatic retransmission enabled.
     * - OFF: Single-shot mode with automatic retransmission disabled.
     */
    enum class RTX : std::uint8_t {
        AUTO = 0x00, // <<< Recommended
        OFF = 0x01
    };
    // * Define default RTX mode
    static constexpr RTX DEFAULT_RTX = RTX::AUTO;

    // === USB Serial Constants ===
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
    enum class SerialBaud : speed_t {
        BAUD_9600 = 9600,
        BAUD_19200 = 19200,
        BAUD_38400 = 38400,
        BAUD_57600 = 57600,
        BAUD_115200 = 115200,
        BAUD_153600 = 153600,
        BAUD_2M = 2000000   // <<< Recommended
    };
    // * Define default Serial baud rate
    static constexpr SerialBaud DEFAULT_SERIAL_BAUD = SerialBaud::BAUD_2M;

    // === Enum Helper Functions ===
    /**
     * @brief Converts an enum value to std::uint8_t.
     *
     * This template function safely converts any enum type to std::uint8_t by first
     * casting to the underlying type and then to std::uint8_t. It's useful for
     * serializing enum values into byte arrays for USB-CAN frame construction.
     *
     * @tparam EnumType The enum type to convert (must be an enum class)
     * @param value The enum value to convert
     * @return std::uint8_t representation of the enum value
     *
     * @example
     * @code
     * auto start_byte = to_byte(Constants::START_BYTE);
     * auto type_byte = to_byte(Type::DATA_FIXED);
     * @endcode
     */
    template<typename EnumType> constexpr std::uint8_t to_byte(EnumType value) {
        return static_cast<std::uint8_t>(
            static_cast<std::underlying_type_t<EnumType> >(value));
    }


    /**
     * @brief Converts a std::uint8_t to an enum value.
     *
     * This template function safely converts std::uint8_t back to an enum type by
     * first casting to the underlying type and then to the enum. It's useful for
     * deserializing byte arrays back into meaningful enum values when parsing
     * received USB-CAN frames.
     *
     * @tparam EnumType The enum type to convert to (must be an enum class)
     * @param value The std::uint8_t value to convert
     * @return EnumType representation of the byte value
     *
     * @warning Ensure the byte value corresponds to a valid enum value to avoid
     *          undefined behavior.
     *
     * @example
     * @code
     * std::uint8_t received_byte = 0xAA;
     * auto constant = from_byte<Constants>(received_byte);
     * if (constant == Constants::START_BYTE) {
     *     // Process start of frame
     * }
     * @endcode
     */
    template<typename EnumType> constexpr EnumType from_byte(std::uint8_t value) {
        return static_cast<EnumType>(
            static_cast<std::underlying_type_t<EnumType> >(value));
    }

    /**
     * @brief Converts SerialBaud enum to speed_t.
     *
     * @param baud The SerialBaud enum value
     * @return speed_t The corresponding speed_t value
     */
    constexpr speed_t to_speed_t(SerialBaud baud) {
        switch (baud) {
        case SerialBaud::BAUD_9600:   return B9600;
        case SerialBaud::BAUD_19200:  return B19200;
        case SerialBaud::BAUD_38400:  return B38400;
        case SerialBaud::BAUD_57600:  return B57600;
        case SerialBaud::BAUD_115200: return B115200;
        case SerialBaud::BAUD_153600: return B153600;
        case SerialBaud::BAUD_2M:     return B2000000;
        default:                      return B2000000;     // <<< Default to 2M
        }
    }

    /**
     * @brief Get the SerialBaud enum from speed_t.
     * @param speed The speed_t value
     * @return SerialBaud The corresponding SerialBaud enum value
     */
    constexpr SerialBaud from_speed_t(speed_t speed) {
        switch (speed) {
        case B9600:     return SerialBaud::BAUD_9600;
        case B19200:    return SerialBaud::BAUD_19200;
        case B38400:    return SerialBaud::BAUD_38400;
        case B57600:    return SerialBaud::BAUD_57600;
        case B115200:   return SerialBaud::BAUD_115200;
        case B153600:  return SerialBaud::BAUD_153600;
        case B2000000:  return SerialBaud::BAUD_2M;
        default:         return SerialBaud::BAUD_2M;   // <<< Default to 2M
        }
    }

    // === Byte Manipulation Helpers ===

    /**
     * @brief Converts an integer to a little-endian byte array.
     * This function converts an unsigned integer value into a std::array of std::uint8_t of specified size, representing the value in little-endian byte order.
     *
     * @note This is useful to give an easy way to write the CAN ID of a data-frame or the filter/mask of a config-frame.
     *
     * @param T The unsigned integer type (e.g., uint8_t, uint16_t, uint32_t, uint64_t)
     * @param N The number of bytes to extract (must be <= sizeof(T))
     * @param value The unsigned integer value to convert
     * @return std::array<std::uint8_t, N> The little-endian byte array representation of the value
     * @throws static_assert if T is not an unsigned integer type or if N is out of range
     * @example
     * @code
     * auto bytes = int_to_bytes_le<uint32_t, 4>(0x123); // bytes = {0x23, 0x01, 0x00, 0x00}
     * auto bytes2 = int_to_bytes_le<uint16_t, 2>(0x1234); // bytes2 = {0x34, 0x12}
     * @endcode
     */
    template<typename T, std::size_t N>
    constexpr std::array<std::uint8_t, N> int_to_bytes_le(T value) {
        static_assert(std::is_unsigned<T>::value, "T must be an unsigned integer type");
        static_assert(N > 0 && N <= sizeof(T), "N must be between 1 and sizeof(T)");
        std::array<std::uint8_t, N> bytes = {};
        for (std::size_t i = 0; i < N; ++i) {
            bytes[i] = static_cast<std::uint8_t>(value & 0xFF);
            value >>= 8;
        }
        return bytes;
    }

    template<typename T>
    constexpr std::array<std::uint8_t, sizeof(T)> int_to_bytes_le(T value) {
        static_assert(std::is_unsigned<T>::value, "T must be an unsigned integer type");
        std::array<std::uint8_t, sizeof(T)> bytes = {};
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            bytes[i] = static_cast<std::uint8_t>(value & 0xFF);
            value >>= 8;
        }
        return bytes;
    }

    /**
     * @brief Converts a little-endian byte array to an unsigned integer.
     * This function converts a span of std::uint8_t representing a little-endian byte sequence
     * into an unsigned integer value.
     * @param T The unsigned integer type to convert to (e.g., uint8_t, uint16_t, uint32_t, uint64_t)
     * @param bytes The little-endian byte array (must be <= sizeof(T))
     * @return T The unsigned integer value represented by the byte array
     * @throws static_assert if T is not an unsigned integer type or if N is out of range
     * @example
     * @code
     * auto value = bytes_to_int_le<uint32_t>(bytes); // value = 0x123
     * @endcode
     */
    template<typename T>
    constexpr T bytes_to_int_le(span<const std::uint8_t> bytes) {
        static_assert(std::is_unsigned<T>::value, "T must be an unsigned integer type");
        T value = 0;
        for (std::size_t i = 0; i < bytes.size() && i < sizeof(T); ++i) {
            value |= (static_cast<T>(bytes[i]) & 0xFF) << (8 * i);
        }
        return value;
    }

    /**
     * @brief Converts an unsigned integer to a big-endian byte array.
     * This function converts an unsigned integer value into a big-endian byte array
     * where the most significant byte is at index 0.
     * @param T The unsigned integer type to convert from (e.g., uint8_t, uint16_t, uint32_t, uint64_t)
     * @param N The number of bytes to output (must be between 1 and sizeof(T))
     * @param value The unsigned integer value to convert
     * @return std::array<std::uint8_t, N> The big-endian byte array representation of the value
     * @throws static_assert if T is not an unsigned integer type or if N is out of range
     * @example
     * @code
     * auto bytes = int_to_bytes_be<uint32_t, 4>(0x7FF); // bytes = {0x00, 0x00, 0x07, 0xFF}
     * auto bytes2 = int_to_bytes_be<uint16_t, 2>(0x1234); // bytes2 = {0x12, 0x34}
     * @endcode
     */
    template<typename T, std::size_t N>
    constexpr std::array<std::uint8_t, N> int_to_bytes_be(T value) {
        static_assert(std::is_unsigned<T>::value, "T must be an unsigned integer type");
        static_assert(N > 0 && N <= sizeof(T), "N must be between 1 and sizeof(T)");
        std::array<std::uint8_t, N> bytes = {};
        for (std::size_t i = 0; i < N; ++i) {
            bytes[N - 1 - i] = static_cast<std::uint8_t>(value & 0xFF);
            value >>= 8;
        }
        return bytes;
    }

    template<typename T>
    constexpr std::array<std::uint8_t, sizeof(T)> int_to_bytes_be(T value) {
        static_assert(std::is_unsigned<T>::value, "T must be an unsigned integer type");
        std::array<std::uint8_t, sizeof(T)> bytes = {};
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            bytes[sizeof(T) - 1 - i] = static_cast<std::uint8_t>(value & 0xFF);
            value >>= 8;
        }
        return bytes;
    }

    /**
     * @brief Converts a big-endian byte array to an unsigned integer.
     * This function converts a span of std::uint8_t representing a big-endian byte sequence
     * into an unsigned integer value, where the most significant byte is at index 0.
     * @param T The unsigned integer type to convert to (e.g., uint8_t, uint16_t, uint32_t, uint64_t)
     * @param bytes The big-endian byte array (must be <= sizeof(T))
     * @return T The unsigned integer value represented by the byte array
     * @throws static_assert if T is not an unsigned integer type or if N is out of range
     * @example
     * @code
     * // bytes = {0x00, 0x00, 0x07, 0xFF}
     * auto value = bytes_to_int_be<uint32_t>(bytes); // value = 0x7FF
     * @endcode
     */
    template<typename T>
    constexpr T bytes_to_int_be(span<const std::uint8_t> bytes) {
        static_assert(std::is_unsigned<T>::value, "T must be an unsigned integer type");
        T value = 0;
        for (std::size_t i = 0; i < bytes.size() && i < sizeof(T); ++i) {
            value = (value << 8) | (static_cast<T>(bytes[i]) & 0xFF);
        }
        return value;
    }


}     // namespace USBCANBridge
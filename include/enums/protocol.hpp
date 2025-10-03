/**
 * @file protocol.hpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief Protocol definitions and helper functions for USB-CAN bridge.
 * @version 0.1
 * @date 2025-10-01
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

#include "../template/frame_traits.hpp"

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
     * <<< Recommended: STD_VARIABLE >>>
     */
    enum class FrameType : std::uint8_t {
        STD_FIXED = 0x01,
        STD_VARIABLE = 0,
        EXT_FIXED = 0x02,
        EXT_VARIABLE = 1
    };
    
    // * Define default Frame Type
    static constexpr FrameType DEFAULT_FRAME_TYPE = FrameType::STD_VARIABLE;

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
     * <<< Recommended: DATA_VARIABLE >>>
     */
    enum class FrameFormat : std::uint8_t {
        DATA_FIXED = 0x01,
        REMOTE_FIXED = 0x02,
        DATA_VARIABLE = 0x00,
        REMOTE_VARIABLE = 0x01
    };
    // * Define default Frame Format
    static constexpr FrameFormat DEFAULT_FRAME_FMT = FrameFormat::DATA_VARIABLE;



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
     * <<< Recommended: 1 Mbps >>>
     */
    enum class CANBaud : std::uint8_t {
        BAUD_1M = 0x01,
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
     * <<< Recommended: NORMAL >>>
     */
    enum class CANMode : std::uint8_t {
        NORMAL = 0x00,
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
     * <<< Recommended: AUTO >>>
     */
    enum class RTX : std::uint8_t { AUTO = 0x00, OFF = 0x01 };
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
     *
     * <<< Recommended: 2 Mbps >>>
     */
    enum class SerialBaud : std::uint32_t {
        BAUD_9600 = 9600,
        BAUD_19200 = 19200,
        BAUD_38400 = 38400,
        BAUD_57600 = 57600,
        BAUD_115200 = 115200,
        BAUD_1228800 = 1228800,
        BAUD_2M = 2000000
    };
    // * Define default Serial baud rate
    static constexpr SerialBaud DEFAULT_SERIAL_BAUD = SerialBaud::BAUD_2M;

    // === Enum Helper Functions ===
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

    // === Byte Manipulation Helpers ===

    /**
     * @brief Dump the content of a frame storage as a hex string.
     * This function converts a byte array to a human-readable hexadecimal string representation,
     * with each byte separated by spaces.
     * @tparam T The frame type (FixedFrame, VariableFrame, ConfigFrame)
     * @param data The byte array to convert
     * @return std::string The hexadecimal string representation of the byte array
     * @example
     * @code
     * auto hex_str = dump_frame<FixedFrame>(frame_data); // hex_str = "AA 55 01 ..."
     * @endcode
     */
    template<typename T>
    std::string dump_frame(const storage_t<T>& data) {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (const auto& byte : data) {
            oss << std::setw(2) << static_cast<int>(byte) << " ";
        }
        std::string result = oss.str();
        if (!result.empty()) {
            result.pop_back(); // Remove trailing space
        }
        return result;
    }

    /**
     * @brief Converts a string of hex values to a byte array.
     * This function converts a std::string containing hexadecimal byte values (e.g., "AA 55 01")
     * into a std::array of std::byte.
     * @param hex_string The input hex string to convert
     * @return std::array<std::byte, N> The resulting byte array
     * @throws std::invalid_argument if the input string is not valid hex
     * @example
     * @code
     * auto bytes = hex_string_to_bytes<FixedFrame>("AA 55 01"); // bytes = {0xAA, 0x55, 0x01}
     * @endcode
     */
    template<typename T>
    storage_t<T> hex_to_bytes(const std::string& hex_string) {
        constexpr std::size_t N = FrameTraits<T>::FRAME_SIZE;
        storage_t<T> bytes = {};
        std::istringstream iss(hex_string);
        std::string byte_str;
        while (iss >> byte_str) {
            if (byte_str.length() != 2) {
                throw std::invalid_argument("Invalid hex byte");
            }
            bytes.push_back(static_cast<std::byte>(std::stoi(byte_str, nullptr, 16)));
        }
        return bytes;
    }

    /**
     * @brief Converts an integer to a little-endian byte array.
     * This function converts an unsigned integer value into a std::array of std::byte of specified size, representing the value in little-endian byte order.
     *
     * @note This is useful to give an easy way to write the CAN ID of a data-frame or the filter/mask of a config-frame.
     *
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
     * @brief Converts a little-endian byte array to an unsigned integer.
     * This function converts a std::array of std::byte representing a little-endian byte sequence
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
    constexpr T bytes_to_int_le(const std::array<std::byte, sizeof(T)>& bytes) {
        static_assert(std::is_unsigned<T>::value, "T must be an unsigned integer type");
        T value = 0;
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            value |= (static_cast<T>(bytes[i]) & 0xFF) << (8 * i);
        }
        return value;
    }

    // === Protocol Static Utiliy Functions ===
    /**
     * @brief Extract the Type base byte from a Type byte.
     * The Type base is a couple of bits that identify the frame as
     * a variable-length frame for the Waveshare USB-CAN adapter protocol.
     * @note The extraction is performed by masking out all but the two highest bits.
     * @param type The Type byte to extract from.
     * @return The extracted Type base byte.
     */
    constexpr Type extract_type_base(std::byte type) {
        return static_cast<Type>(
            static_cast<std::uint8_t>(type) & 0xC0);
    }
    /**
     * @brief Extract the FrameType from a Type byte.
     * The FrameType is a single bit that indicates whether the frame is
     * standard (STD) or extended (EXT).
     * @note The extraction is performed by right-shifting to select the fifth bit
     * and masking out all other bits.
     * @param type The Type byte to extract from.
     * @return The extracted FrameType.
     */
    constexpr FrameType extract_frame_type(std::byte type) {
        return static_cast<FrameType>(
            (static_cast<std::uint8_t>(type) >> 5) & 1);
    }
    /**
     * @brief Extract the FrameFormat from a Type byte.
     * The FrameFormat is a single bit that indicates whether the frame is
     * a data frame or a remote frame.
     * @note The extraction is performed by right-shifting to select the fourth bit
     * and masking out all other bits.
     * @param type The Type byte to extract from.
     * @return The extracted FrameFormat.
     */
    constexpr FrameFormat extract_frame_format(std::byte type) {
        return static_cast<FrameFormat>(
            (static_cast<std::uint8_t>(type) >> 4) & 1);
    }
    /**
     * @brief Extract the DLC from a Type byte.
     * The DLC (Data Length Code) is represented by the lowest four bits of the Type byte,
     * which indicates the number of data bytes in the frame.
     * @note The extraction is performed by masking out all but the four lowest bits using a
     * bitwise AND operation with `0x0F` (`0b1111`).
     * @param type The Type byte to extract from.
     * @return The extracted DLC (0-8).
     */
    constexpr std::size_t extract_dlc(std::byte type) {
        return static_cast<std::size_t>(static_cast<std::uint8_t>(type) & 0x0F);
    }
    /**
     * @brief Calculate the Type byte for a VariableFrame based on its properties.
     * This function determines the appropriate Type byte for a VariableFrame
     * based on the frame's FrameType, FrameFormat, and data length code (DLC).
     * @param frame_type The FrameType of the VariableFrame
     * @param frame_fmt The FrameFormat of the VariableFrame
     * @param dlc The data length code (DLC) of the VariableFrame
     * @note The Type byte is constructed as follows:
     *
     * - Bits 7-6: Type base byte (0xC0 for VariableFrame)
     *
     * - Bit 5: FrameType (0 for STD, 1 for EXT)
     *
     * - Bit 4: FrameFormat (0 for DATA, 1 for REMOTE)
     *
     * - Bits 3-0: DLC (0-8)
     *
     * @see extract_type_base, extract_frame_type, extract_frame_format,
       extract_dlc to know the reason behind each bit manipulation.
     * @warning We assume that the caller has already validated each parameter.
     * @return The calculated Type byte for the VariableFrame
     */
    template<typename T>
    constexpr std::enable_if_t<is_variable_frame_v<T>, std::byte>
    compute_type(FrameType frame_type, FrameFormat frame_fmt, std::size_t dlc) {
        // Combine all bit operations into a single expression
        return static_cast<std::byte>(
            static_cast<std::uint8_t>(Type::DATA_VARIABLE) |
            (static_cast<std::uint8_t>(frame_type) << 5) |
            (static_cast<std::uint8_t>(frame_fmt) << 4) |
            (static_cast<std::uint8_t>(dlc) & 0x0F)
        );
    }

}     // namespace USBCANBridge
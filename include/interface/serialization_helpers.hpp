/**
 * @file serialization_helpers.hpp
 * @brief Pure static helper classes for frame serialization
 * @version 3.0
 * @date 2025-10-09
 *
 * State-First Architecture Helpers:
 * - ChecksumHelper: Compute and validate checksums on raw buffers
 * - VarTypeHelper: Encode/decode VariableFrame TYPE byte
 *
 * These are pure static classes (no CRTP, no state) that work on
 * raw byte buffers. They are used during serialization/deserialization.
 *
 * @copyright Copyright (c) 2025
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <numeric>
#include <boost/core/span.hpp>
#include "../enums/protocol.hpp"

using namespace boost;

namespace waveshare {

    /**
     * @brief Static helper for checksum computation and validation
     *
     * Pure static class with no state. All methods work on raw buffers
     * using layout offsets provided by the caller.
     */
    class ChecksumHelper {
        public:
            /**
             * @brief Compute checksum from a byte range
             *
             * Sums all bytes in the range [start, end) and returns the lower 8 bits.
             *
             * @param data Buffer containing the data
             * @param start Start offset (inclusive)
             * @param end End offset (exclusive)
             * @return std::uint8_t The computed checksum (sum & 0xFF)
             *
             * @example
             * @code
             * // For FixedFrame: checksum from TYPE to RESERVED (bytes 2-18)
             * auto checksum = ChecksumHelper::compute(buffer, 2, 19);
             * @endcode
             */
            static std::uint8_t compute(span<const std::uint8_t> data,
                std::size_t start,
                std::size_t end) {
                if (end > data.size() || start >= end) {
                    return 0x00; // Invalid range
                }

                std::uint16_t sum = std::accumulate(
                    data.begin() + start,
                    data.begin() + end,
                    std::uint16_t(0),
                    [](std::uint16_t acc, std::uint8_t b) {
                        return acc + static_cast<std::uint8_t>(b);
                    }
                );

                return static_cast<std::uint8_t>(sum & 0xFF);
            }

            /**
             * @brief Validate checksum in a buffer
             *
             * Computes checksum from [start, end) and compares it to the
             * stored checksum at checksum_pos.
             *
             * @param buffer Buffer containing the frame
             * @param checksum_pos Offset of the stored checksum byte
             * @param start Start of checksum range (inclusive)
             * @param end End of checksum range (exclusive)
             * @return bool True if checksums match, false otherwise
             *
             * @example
             * @code
             * // For FixedFrame: validate checksum at byte 19, range 2-19
             * bool valid = ChecksumHelper::validate(buffer, 19, 2, 19);
             * @endcode
             */
            static bool validate(span<const std::uint8_t> buffer,
                std::size_t checksum_pos,
                std::size_t start,
                std::size_t end) {
                if (checksum_pos >= buffer.size()) {
                    return false; // Invalid checksum position
                }

                std::uint8_t stored = buffer[checksum_pos];
                std::uint8_t computed = compute(buffer, start, end);

                return stored == computed;
            }

            /**
             * @brief Compute and write checksum to buffer
             *
             * Computes checksum from [start, end) and writes it to checksum_pos.
             * This modifies the buffer.
             *
             * @param buffer Mutable buffer to write checksum to
             * @param checksum_pos Offset where checksum should be written
             * @param start Start of checksum range (inclusive)
             * @param end End of checksum range (exclusive)
             *
             * @example
             * @code
             * // For FixedFrame: compute and write checksum at byte 19
             * ChecksumHelper::write(buffer, 19, 2, 19);
             * @endcode
             */
            static void write(span<std::uint8_t> buffer,
                std::size_t checksum_pos,
                std::size_t start,
                std::size_t end) {
                if (checksum_pos >= buffer.size()) {
                    return; // Invalid position
                }

                buffer[checksum_pos] = compute(buffer, start, end);
            }
    };

    /**
     * @brief Static helper for VariableFrame TYPE byte encoding/decoding
     *
     * The TYPE byte structure (8 bits):
     * - Bits 7-6: 0xC0 (constant DATA_VARIABLE marker)
     * - Bit 5: CAN version (0=STD, 1=EXT)
     * - Bit 4: Format (0=DATA, 1=REMOTE)
     * - Bits 3-0: DLC (0-8 data bytes)
     */
    class VarTypeHelper {
        public:
            /**
             * @brief Encode TYPE byte from components
             *
             * Constructs the TYPE byte by combining:
             * - Base constant (0xC0)
             * - CAN version bit (STD=0, EXT=1)
             * - Format bit (DATA=0, REMOTE=1)
             * - DLC bits (0-8)
             *
             * @param can_vers CANVersion (STD_VARIABLE or EXT_VARIABLE)
             * @param format Format (DATA_VARIABLE or REMOTE_VARIABLE)
             * @param dlc Data Length Code (0-8)
             * @return std::uint8_t The encoded TYPE byte
             *
             * @example
             * @code
             * // Standard ID, data frame, 4 bytes
             * auto type = VarTypeHelper::compute_type(
             *     CANVersion::STD_VARIABLE,
             *     Format::DATA_VARIABLE,
             *     4
             * );
             * // Result: 0xC4 (11000100)
             * @endcode
             */
            static std::uint8_t compute_type(CANVersion can_vers,
                Format format,
                std::size_t dlc) {
                // Start with base DATA_VARIABLE (0xC0)
                std::uint8_t type_byte = to_byte(Type::DATA_VARIABLE);

                // Set bit 5 for extended CAN ID
                if (can_vers == CANVersion::EXT_VARIABLE) {
                    type_byte |= 0x20; // Set bit 5
                }

                // Set bit 4 for remote frame
                if (format == Format::REMOTE_VARIABLE) {
                    type_byte |= 0x10; // Set bit 4
                }

                // Set bits 3-0 for DLC (mask to ensure 0-15 range)
                type_byte |= (static_cast<std::uint8_t>(dlc) & 0x0F);

                return type_byte;
            }

            /**
             * @brief Decode TYPE byte into components
             *
             * Extracts CAN version, format, and DLC from TYPE byte.
             *
             * @param type_byte The TYPE byte to decode
             * @return struct { CANVersion can_vers; Format format; std::size_t dlc; }
             *
             * @example
             * @code
             * auto [can_vers, format, dlc] = VarTypeHelper::parse_type(0xC4);
             * // can_vers = STD_VARIABLE, format = DATA_VARIABLE, dlc = 4
             * @endcode
             */
            struct TypeComponents {
                CANVersion can_vers;
                Format format;
                std::size_t dlc;
            };

            static TypeComponents parse_type(std::uint8_t type_byte) {
                TypeComponents result;

                // Extract CAN version from bit 5
                bool is_extended = (type_byte & 0x20) != 0;
                result.can_vers = is_extended ?
                    CANVersion::EXT_VARIABLE :
                    CANVersion::STD_VARIABLE;

                // Extract format from bit 4
                bool is_remote = (type_byte & 0x10) != 0;
                result.format = is_remote ?
                    Format::REMOTE_VARIABLE :
                    Format::DATA_VARIABLE;

                // Extract DLC from bits 3-0
                result.dlc = static_cast<std::size_t>(type_byte & 0x0F);

                return result;
            }

            /**
             * @brief Quick check if TYPE byte indicates extended ID
             *
             * @param type_byte The TYPE byte to check
             * @return bool True if bit 5 is set (extended ID)
             *
             * @example
             * @code
             * if (VarTypeHelper::is_extended(type_byte)) {
             *     // Handle 29-bit ID
             * }
             * @endcode
             */
            static bool is_extended(std::uint8_t type_byte) {
                return (type_byte & 0x20) != 0;
            }

            /**
             * @brief Quick check if TYPE byte indicates remote frame
             *
             * @param type_byte The TYPE byte to check
             * @return bool True if bit 4 is set (remote frame)
             */
            static bool is_remote(std::uint8_t type_byte) {
                return (type_byte & 0x10) != 0;
            }

            /**
             * @brief Extract DLC from TYPE byte
             *
             * @param type_byte The TYPE byte
             * @return std::size_t The DLC (0-8)
             */
            static std::size_t get_dlc(std::uint8_t type_byte) {
                return static_cast<std::size_t>(type_byte & 0x0F);
            }
    };

}  // namespace waveshare

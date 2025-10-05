/**
 * @file frame_traits.hpp
 * @brief Simplified frame traits implementation for USB-CAN adapter frame types.
 *
 * This file provides a streamlined template-based traits system focused on
 * storage types and byte layout information. The design includes:
 *
 * ## Key Features:
 *
 * ### 1. Simplified Interface
 * - Primary template uses void types to prevent accidental usage
 * - Focus on StorageType and byte layout constants
 * - Elimination of complex type aliases (IDPair, PayloadPair, etc.)
 *
 * ### 2. Layout Centralization
 * - All byte offsets and sizes centralized in Layout nested structs
 * - Compile-time constants for all frame field positions
 * - No more magic numbers scattered throughout the codebase
 *
 * ### 3. Type Safety
 * - SFINAE-friendly frame type checking
 * - Compile-time validation of frame layouts
 * - Clear separation between data frames and config frames
 *
 * ## Specializations:
 * - FrameTraits<FixedFrame>: 20-byte fixed frames with arrays and layout
 * - FrameTraits<VariableFrame>: Variable frames with vectors and dynamic layout
 * - FrameTraits<ConfigFrame>: 20-byte configuration frames with config layout
 *
 * ## Usage Examples:
 * @code
 * // Type checking
 * static_assert(is_data_frame_v<FixedFrame>);
 * static_assert(is_config_frame_v<ConfigFrame>);
 *
 * // Layout access
 * using Layout = layout_t<FixedFrame>;
 * constexpr auto id_offset = Layout::ID_OFFSET;
 *
 * // Storage access
 * using Storage = typename frame_traits_t<FixedFrame>::StorageType;
 * @endcode
 *
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @version 2.0
 * @date 2025-09-29
 * @copyright Copyright (c) 2025
 */

#pragma once

#include <array>
#include <cstddef>
#include <type_traits>
#include <vector>
#include <boost/core/span.hpp>

using namespace boost;

/**
 * @namespace USBCANBridge
 * @brief Namespace containing all USB-CAN bridge related functionality.
 */
namespace USBCANBridge {

// Forward declarations
    class FixedFrame;
    class VariableFrame;
    class ConfigFrame;

/**
 * @brief Primary template for frame traits.
 *
 * This template trait provides compile-time information about frame structure,
 * storage requirements, and type definitions for different frame types.
 * It uses CRTP (Curiously Recurring Template Pattern) to provide specialized
 * implementations for each frame type.
 *
 * The primary template defines the expected interface that all specializations
 * must implement, serving as both documentation and a concept-like constraint.
 *
 * @tparam Derived The derived frame type (FixedFrame, VariableFrame,
 * ConfigFrame)
 *
 * @note This primary template intentionally triggers compilation errors if used
 *       directly without specialization. All concrete frame types must provide
 *       explicit specializations with the required interface.
 */
    template<typename Frame>
    struct FrameTraits {
        static constexpr std::size_t FRAME_SIZE = 0;
        std::array<std::byte, 0> frame_buffer = {}; // Intentionally empty to prevent usage
        using StorageType = void; // Intentionally void to prevent usage
        struct Layout {}; // Empty layout to prevent usage
    };

/**
 * @brief Enhanced FixedFrame traits with complete layout information.
 *
 * Provides traits for fixed-size frames with 20-byte total size,
 * 4-byte CAN ID, and 8-byte data payload. Uses std::array for
 * compile-time size guarantees and efficient stack allocation.
 */
    template<>
    struct FrameTraits<FixedFrame> {
        static constexpr std::size_t FRAME_SIZE = 20;
        alignas(8) std::array<std::byte, FRAME_SIZE> frame_buffer = {};
        using StorageType = std::array<std::byte, FRAME_SIZE>;


        /**
         * @brief 20-byte Fixed frame byte layout.
         */
        struct Layout {
            static constexpr std::size_t START_OFFSET = 0;
            static constexpr std::size_t HEADER_OFFSET = 1;
            static constexpr std::size_t TYPE_OFFSET = 2;
            static constexpr std::size_t FRAME_TYPE_OFFSET = 3;
            static constexpr std::size_t FORMAT_OFFSET = 4;
            static constexpr std::size_t ID_OFFSET = 5;
            static constexpr std::size_t ID_SIZE = 4;
            static constexpr std::size_t DLC_OFFSET = 9;
            static constexpr std::size_t DATA_OFFSET = 10;
            static constexpr std::size_t DATA_SIZE = 8;
            static constexpr std::size_t RESERVED_OFFSET = 18;
            static constexpr std::size_t CHECKSUM_OFFSET = 19;
        };
    };

/**
 * @brief Enhanced VariableFrame traits with dynamic layout helpers.
 *
 * Provides traits for variable-size frames with dynamic allocation,
 * supporting both standard (11-bit) and extended (29-bit) CAN IDs,
 * and variable data payload up to 8 bytes. Uses std::vector for
 * dynamic sizing.
 */
    template<>
    struct FrameTraits<VariableFrame> {
        static constexpr std::size_t MAX_FRAME_SIZE = 15;
        static constexpr std::size_t MIN_FRAME_SIZE = 5;
        static constexpr std::size_t MAX_DATA_SIZE = 8;
        static constexpr std::size_t MIN_DATA_SIZE = 0;
        static constexpr std::size_t MAX_ID_SIZE = 4; // Extended ID
        static constexpr std::size_t MIN_ID_SIZE = 2; // Standard ID
        alignas(8) std::vector<std::byte> frame_buffer = std::vector<std::byte>(MIN_FRAME_SIZE);
        using StorageType = span<std::byte>; // Dynamic size


        /**
         * @brief Dynamic layout with helper functions for runtime calculations.
         */
        struct Layout {
            static constexpr std::size_t START_OFFSET = 0;
            static constexpr std::size_t TYPE_OFFSET = 1;
            static constexpr std::size_t ID_OFFSET = 2;

            // Dynamic offsets - helper functions
            static constexpr std::size_t data_offset(bool extended_id) {
                return ID_OFFSET + (extended_id ? 4 : 2);
            }

            static constexpr std::size_t end_offset(bool extended_id, std::size_t data_len) {
                return data_offset(extended_id) + data_len;
            }

            static constexpr std::size_t frame_size(bool extended_id, std::size_t data_len) {
                return end_offset(extended_id, data_len) + 1; // +1 for END byte
            }
        };
    };

/**
 * @brief Enhanced ConfigFrame traits with configuration layout.
 *
 * Provides traits for configuration frames with 20-byte total size
 * and configuration payload. Used for setting baud rates, filters,
 * modes, and other adapter parameters.
 */
    template<>
    struct FrameTraits<ConfigFrame> {
        static constexpr std::size_t FRAME_SIZE = 20;
        alignas(8) std::array<std::byte, FRAME_SIZE> frame_buffer = {};
        using StorageType = span<std::byte, FRAME_SIZE>;


        /**
         * @brief Configuration-specific byte layout.
         */
        struct Layout {
            static constexpr std::size_t START_OFFSET = 0;
            static constexpr std::size_t HEADER_OFFSET = 1;
            static constexpr std::size_t TYPE_OFFSET = 2;
            static constexpr std::size_t BAUD_OFFSET = 3;
            static constexpr std::size_t FRAME_TYPE_OFFSET = 4;
            static constexpr std::size_t FILTER_OFFSET = 5;
            static constexpr std::size_t FILTER_SIZE = 4;
            static constexpr std::size_t MASK_OFFSET = 9;
            static constexpr std::size_t MASK_SIZE = 4;
            static constexpr std::size_t MODE_OFFSET = 13;
            static constexpr std::size_t AUTO_RTX_OFFSET = 14;
            static constexpr std::size_t RESERVED_OFFSET = 15;
            static constexpr std::size_t RESERVED_SIZE = 4;
            static constexpr std::size_t CHECKSUM_OFFSET = 19;
        };
    };

// Convenient aliases
    template<typename T> using frame_traits_t = FrameTraits<T>;
    template<typename T> using layout_t = typename FrameTraits<T>::Layout;
    template<typename T> using storage_t = typename FrameTraits<T>::StorageType;

// Frame categorization for SFINAE
    template<typename T>
    struct is_data_frame : std::bool_constant<
            std::is_same_v<T, FixedFrame>|| std::is_same_v<T, VariableFrame>
        > {};

    template<typename T>
    struct is_config_frame : std::bool_constant<std::is_same_v<T, ConfigFrame> > {};

    template<typename T>
    struct is_variable_frame : std::bool_constant<std::is_same_v<T, VariableFrame> > {};

    template<typename T>
    struct is_fixed_frame : std::bool_constant<std::is_same_v<T, FixedFrame> > {};

    template<typename T>
    struct has_checksum : std::bool_constant<
            std::is_same_v<T, FixedFrame>|| std::is_same_v<T, ConfigFrame>
        > {};

    template<typename T>
    inline constexpr bool is_data_frame_v = is_data_frame<T>::value;

    template<typename T>
    inline constexpr bool is_config_frame_v = is_config_frame<T>::value;

    template<typename T>
    inline constexpr bool is_variable_frame_v = is_variable_frame<T>::value;

    template<typename T>
    inline constexpr bool is_fixed_frame_v = is_fixed_frame<T>::value;

    template<typename T>
    inline constexpr bool has_checksum_v = has_checksum<T>::value;
} // namespace USBCANBridge
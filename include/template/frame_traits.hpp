/**
 * @file frame_traits.hpp
 * @brief Pure compile-time frame traits for USB-CAN adapter frame types.
 *
 * This file provides compile-time metadata for frame structure and layout.
 * The design follows the state-first architecture where frames store their
 * state internally and generate protocol buffers on demand via serialization.
 *
 * ## Key Features:
 *
 * ### 1. Pure Compile-Time Metadata
 * - No runtime state (no frame_buffer, no mutable layout)
 * - All layout information as static constexpr constants
 * - Frame characteristics as compile-time boolean flags
 *
 * ### 2. Layout Structures
 * - Separate layout structs for each frame type
 * - Fixed layouts: All offsets are static constexpr
 * - Variable layouts: Constexpr functions for runtime size calculations
 *
 * ### 3. Type Safety
 * - SFINAE-friendly frame type predicates
 * - Compile-time frame categorization
 * - Clear separation between data frames and config frames
 *
 * ## Specializations:
 * - FrameTraits<FixedFrame>: 20-byte fixed frames with FixedFrameLayout
 * - FrameTraits<VariableFrame>: 5-15 byte variable frames with VariableFrameLayout
 * - FrameTraits<ConfigFrame>: 20-byte configuration frames with ConfigFrameLayout
 *
 * ## Usage Examples:
 * @code
 * // Layout access (compile-time constants)
 * using Layout = FrameTraits<FixedFrame>::Layout;
 * buffer[Layout::START] = 0xAA;
 * buffer[Layout::ID] = id_byte;
 *
 * // Size constants
 * constexpr size_t size = FrameTraits<FixedFrame>::FRAME_SIZE;
 *
 * // Type predicates
 * static_assert(is_data_frame_v<FixedFrame>);
 * static_assert(has_checksum_v<ConfigFrame>);
 * @endcode
 *
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @version 3.0 - State-First Architecture
 * @date 2025-10-09
 * @copyright Copyright (c) 2025
 */

#pragma once

#include <cstddef>
#include <type_traits>

namespace USBCANBridge {

    // Forward declarations
    class FixedFrame;
    class VariableFrame;
    class ConfigFrame;

    // === Layout Definitions ===

    /**
     * @brief Fixed-size frame layout (20 bytes)
     * Used by FixedFrame - all offsets are compile-time constants
     */
    struct FixedFrameLayout {
        // Protocol structure byte offsets
        static constexpr std::size_t START = 0;
        static constexpr std::size_t HEADER = 1;
        static constexpr std::size_t TYPE = 2;
        static constexpr std::size_t CAN_VERS = 3;
        static constexpr std::size_t FORMAT = 4;
        static constexpr std::size_t ID = 5;
        static constexpr std::size_t DLC = 9;
        static constexpr std::size_t DATA = 10;
        static constexpr std::size_t RESERVED = 18;
        static constexpr std::size_t CHECKSUM = 19;

        // Field sizes
        static constexpr std::size_t ID_SIZE = 4;
        static constexpr std::size_t DATA_SIZE = 8;

        // Checksum range (TYPE to RESERVED inclusive)
        static constexpr std::size_t CHECKSUM_START = TYPE;
        static constexpr std::size_t CHECKSUM_END = RESERVED;
    };

    /**
     * @brief Variable-size frame layout (5-15 bytes)
     * Used by VariableFrame - some offsets computed at runtime
     */
    struct VariableFrameLayout {
        // Fixed offsets
        static constexpr std::size_t START = 0;
        static constexpr std::size_t TYPE = 1;
        static constexpr std::size_t ID = 2;

        // Runtime-dependent offsets (depend on ID size and DLC)
        static constexpr std::size_t id_size(bool is_extended) {
            return is_extended ? 4 : 2;
        }

        static constexpr std::size_t data_offset(bool is_extended) {
            return ID + id_size(is_extended);
        }

        static constexpr std::size_t end_offset(bool is_extended, std::size_t dlc) {
            return data_offset(is_extended) + dlc;
        }

        // Frame size calculation
        static constexpr std::size_t frame_size(bool is_extended, std::size_t dlc) {
            return 1 + 1 + id_size(is_extended) + dlc + 1; // START + TYPE + ID + DATA + END
        }
    };

    /**
     * @brief Config frame layout (20 bytes)
     * Used by ConfigFrame - all offsets are compile-time constants
     */
    struct ConfigFrameLayout {
        static constexpr std::size_t START = 0;
        static constexpr std::size_t HEADER = 1;
        static constexpr std::size_t TYPE = 2;
        static constexpr std::size_t BAUD = 3;
        static constexpr std::size_t CAN_VERS = 4;
        static constexpr std::size_t FILTER = 5;
        static constexpr std::size_t MASK = 9;
        static constexpr std::size_t MODE = 13;
        static constexpr std::size_t AUTO_RTX = 14;
        static constexpr std::size_t RESERVED = 15;
        static constexpr std::size_t CHECKSUM = 19;

        // Field sizes
        static constexpr std::size_t FILTER_SIZE = 4;
        static constexpr std::size_t MASK_SIZE = 4;
        static constexpr std::size_t RESERVED_SIZE = 4;

        // Checksum range (TYPE to last RESERVED byte inclusive)
        static constexpr std::size_t CHECKSUM_START = TYPE;
        static constexpr std::size_t CHECKSUM_END = RESERVED + RESERVED_SIZE - 1;
    };

    // === Frame Traits Specializations ===

    /**
     * @brief Primary template - not instantiable (forces specialization)
     */
    template<typename Frame>
    struct FrameTraits {
        static_assert(std::is_same_v<Frame, void>,
            "FrameTraits must be specialized for concrete frame types (FixedFrame, VariableFrame, ConfigFrame)");
    };

    /**
     * @brief FixedFrame traits - 20 bytes fixed size with checksum
     */
    template<>
    struct FrameTraits<FixedFrame> {
        using Layout = FixedFrameLayout;

        // Size constants
        static constexpr std::size_t FRAME_SIZE = 20;
        static constexpr std::size_t MIN_FRAME_SIZE = 20;
        static constexpr std::size_t MAX_FRAME_SIZE = 20;
        static constexpr std::size_t MAX_DATA_SIZE = 8;

        // Frame characteristics
        static constexpr bool IS_VARIABLE_SIZE = false;
        static constexpr bool HAS_CHECKSUM = true;
        static constexpr bool IS_DATA_FRAME = true;
        static constexpr bool IS_CONFIG_FRAME = false;
    };

    /**
     * @brief VariableFrame traits - 5 to 15 bytes dynamic size, no checksum
     */
    template<>
    struct FrameTraits<VariableFrame> {
        using Layout = VariableFrameLayout;

        // Size constants
        static constexpr std::size_t MIN_FRAME_SIZE = 5; // START + TYPE + ID(2) + END
        static constexpr std::size_t MAX_FRAME_SIZE = 15; // START + TYPE + ID(4) + DATA(8) + END
        static constexpr std::size_t MAX_DATA_SIZE = 8;

        // Frame characteristics
        static constexpr bool IS_VARIABLE_SIZE = true;
        static constexpr bool HAS_CHECKSUM = false;
        static constexpr bool IS_DATA_FRAME = true;
        static constexpr bool IS_CONFIG_FRAME = false;
    };

    /**
     * @brief ConfigFrame traits - 20 bytes fixed size with checksum
     */
    template<>
    struct FrameTraits<ConfigFrame> {
        using Layout = ConfigFrameLayout;

        // Size constants
        static constexpr std::size_t FRAME_SIZE = 20;
        static constexpr std::size_t MIN_FRAME_SIZE = 20;
        static constexpr std::size_t MAX_FRAME_SIZE = 20;

        // Frame characteristics
        static constexpr bool IS_VARIABLE_SIZE = false;
        static constexpr bool HAS_CHECKSUM = true;
        static constexpr bool IS_DATA_FRAME = false;
        static constexpr bool IS_CONFIG_FRAME = true;
    };

    // === Helper Type Aliases ===

    /**
     * @brief Alias for accessing FrameTraits
     * Usage: using Traits = traits_t<FixedFrame>;
     */
    template<typename Frame>
    using traits_t = FrameTraits<Frame>;

    /**
     * @brief Alias for accessing Layout type from FrameTraits
     * Usage: using Layout = layout_t<FixedFrame>;
     */
    template<typename Frame>
    using layout_t = typename FrameTraits<Frame>::Layout;

    // === Type Predicates ===

    /**
     * @brief Check if frame has variable size
     */
    template<typename Frame>
    inline constexpr bool is_variable_frame_v = FrameTraits<Frame>::IS_VARIABLE_SIZE;

    /**
     * @brief Check if frame has fixed size
     */
    template<typename Frame>
    inline constexpr bool is_fixed_frame_v = !is_variable_frame_v<Frame>&&
        FrameTraits<Frame>::IS_DATA_FRAME;

    /**
     * @brief Check if frame is a data frame (FixedFrame or VariableFrame)
     */
    template<typename Frame>
    inline constexpr bool is_data_frame_v = FrameTraits<Frame>::IS_DATA_FRAME;

    /**
     * @brief Check if frame is a config frame (ConfigFrame)
     */
    template<typename Frame>
    inline constexpr bool is_config_frame_v = FrameTraits<Frame>::IS_CONFIG_FRAME;

    /**
     * @brief Check if frame has checksum (FixedFrame or ConfigFrame)
     */
    template<typename Frame>
    inline constexpr bool has_checksum_v = FrameTraits<Frame>::HAS_CHECKSUM;

}  // namespace USBCANBridge
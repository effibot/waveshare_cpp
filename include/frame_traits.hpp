/**
 * @file frame_traits.hpp
 * @brief Hybrid frame traits implementation for USB-CAN adapter frame types.
 *
 * This file provides a hybrid template-based traits system that combines the
 * best aspects of both interface documentation and type safety. The design
 * includes:
 *
 * ## Key Features:
 *
 * ### 1. Interface Documentation
 * - Primary template clearly documents the expected interface
 * - All required constants and type aliases are explicitly listed
 * - Serves as both documentation and a concept-like constraint
 *
 * ### 2. Type Safety
 * - Primary template uses void types to prevent accidental usage
 * - Compile-time validation through static_assert in specializations
 * - SFINAE-friendly type checking without triggering errors
 *
 * ### 3. Modern C++ Utilities
 * - Convenient alias templates (frame_traits_t<T>)
 * - Variable templates for direct constant access (frame_size_v<T>)
 * - Type traits for template constraints (is_frame_type_v<T>)
 * - Comprehensive validation (is_valid_frame_traits_v<T>)
 *
 * ## Specializations:
 * - FrameTraits<FixedFrame>: 20-byte fixed frames with arrays
 * - FrameTraits<VariableFrame>: Variable frames with vectors/variants
 * - FrameTraits<ConfigFrame>: 20-byte configuration frames
 *
 * ## Usage Examples:
 * @code
 * // Type checking
 * static_assert(is_frame_type_v<FixedFrame>);
 *
 * // Convenient access
 * using Traits = frame_traits_t<FixedFrame>;
 * constexpr auto size = frame_size_v<FixedFrame>;
 *
 * // Template constraints
 * template<typename T>
 * std::enable_if_t<is_frame_type_v<T>, void> process_frame();
 * @endcode
 *
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @version 1.1
 * @date 2025-09-21
 * @copyright Copyright (c) 2025
 */

#pragma once

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

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
    template<typename Derived> struct FrameTraits {
        // Interface documentation - these members must be provided by specializations

        /// Total frame size in bytes (or MAX_FRAME_SIZE for variable frames)
        /// Must be: static constexpr std::size_t FRAME_SIZE = N;
        static constexpr std::size_t FRAME_SIZE = 0;

        /// CAN ID field size in bytes (or MAX_ID_SIZE for variable frames)
        /// Must be: static constexpr std::size_t ID_SIZE = N;
        static constexpr std::size_t ID_SIZE = 0;

        /// Data payload size in bytes (or MAX_DATA_SIZE for variable frames)
        /// Must be: static constexpr std::size_t DATA_SIZE = N;
        static constexpr std::size_t DATA_SIZE = 0;

        /// Storage type for the entire frame (std::array for fixed, std::vector for
        /// variable) Must be: using StorageType = /* appropriate container type */;
        using StorageType = void;

        /// Type for CAN ID storage
        /// Must be: using IDType = /* appropriate ID type */;
        using IDType = void;

        /// Type for data payload storage
        /// Must be: using DataType = /* appropriate data type */;
        using DataType = void;

        /// Pair type for data payload with DLC (Data Length Code)
        /// Must be: using PayloadPair = std::pair<DataType, std::size_t>;
        using PayloadPair = void;

        /// Pair type for CAN ID with length information
        /// Must be: using IDPair = std::pair<IDType, std::size_t>;
        using IDPair = void;
    };

/**
 * @brief Frame traits specialization for FixedFrame.
 *
 * Provides traits for fixed-size frames with 20-byte total size,
 * 4-byte CAN ID, and 8-byte data payload. Uses std::array for
 * compile-time size guarantees and efficient stack allocation.
 */
    template<> struct FrameTraits<FixedFrame> {
        /// Total frame size in bytes (including header, ID, data, checksum)
        static constexpr std::size_t FRAME_SIZE = 20;

        /// CAN ID field size in bytes
        static constexpr std::size_t ID_SIZE = 4;

        /// Data payload size in bytes
        static constexpr std::size_t DATA_SIZE = 8;

        /// Storage type for the entire frame (fixed-size array)
        using StorageType = std::array<std::byte, FRAME_SIZE>;

        /// Type for CAN ID storage (4-byte array)
        using IDType = std::array<std::byte, ID_SIZE>;

        /// Type for data payload storage (8-byte array)
        using DataType = std::array<std::byte, DATA_SIZE>;

        /// Pair type for data payload with DLC (Data Length Code)
        using PayloadPair = std::pair<DataType, std::size_t>;

        /// Pair type for CAN ID with length information
        using IDPair = std::pair<IDType, std::size_t>;

        private:
            // Compile-time interface validation
            static_assert(FRAME_SIZE == 20, "FRAME_SIZE must be 20 bytes");
            static_assert(ID_SIZE > 0, "ID_SIZE must be positive");
            static_assert(DATA_SIZE > 0, "DATA_SIZE must be positive");
            static_assert(!std::is_void_v<StorageType>, "StorageType must be defined");
            static_assert(!std::is_void_v<IDType>, "IDType must be defined");
            static_assert(!std::is_void_v<DataType>, "DataType must be defined");
            static_assert(!std::is_void_v<PayloadPair>, "PayloadPair must be defined");
            static_assert(!std::is_void_v<IDPair>, "IDPair must be defined");
    };

/**
 * @brief Frame traits specialization for VariableFrame.
 *
 * Provides traits for variable-size frames with dynamic allocation,
 * supporting both standard (11-bit) and extended (29-bit) CAN IDs,
 * and variable data payload up to 8 bytes. Uses std::vector for
 * dynamic sizing and std::variant for flexible ID types.
 */
    template<> struct FrameTraits<VariableFrame> {
        /// Maximum frame size in bytes
        static constexpr std::size_t MAX_FRAME_SIZE = 15;

        /// Maximum CAN ID field size in bytes (for extended ID)
        static constexpr std::size_t MAX_ID_SIZE = 4;

        /// Maximum data payload size in bytes
        static constexpr std::size_t MAX_DATA_SIZE = 8;

        /// Minimum frame size in bytes (for empty payload)
        static constexpr std::size_t MIN_FRAME_SIZE = 5;

        /// Storage type for the entire frame (dynamic vector)
        using StorageType = std::vector<std::byte>;

        /// Type for CAN ID storage (variant for standard/extended ID)
        using IDType = std::variant<std::array<std::byte, 2>, // Standard ID (11-bit)
                std::array<std::byte, 4>                // Extended ID (29-bit)
        >;

        /// Type for data payload storage (dynamic vector)
        using DataType = std::vector<std::byte>;

        /// Pair type for data payload with DLC (Data Length Code)
        using PayloadPair = std::pair<DataType, std::size_t>;

        /// Pair type for CAN ID with extended flag
        using IDPair = std::pair<IDType, std::size_t>;

        private:
            // Compile-time interface validation
            static_assert(MAX_FRAME_SIZE > 0, "MAX_FRAME_SIZE must be positive");
            static_assert(MAX_ID_SIZE > 0, "MAX_ID_SIZE must be positive");
            static_assert(MAX_DATA_SIZE > 0, "MAX_DATA_SIZE must be positive");
            static_assert(MIN_FRAME_SIZE > 0 && MIN_FRAME_SIZE < MAX_FRAME_SIZE,
            "MIN_FRAME_SIZE must be positive and less than MAX_FRAME_SIZE");
            static_assert(!std::is_void_v<StorageType>, "StorageType must be defined");
            static_assert(!std::is_void_v<IDType>, "IDType must be defined");
            static_assert(!std::is_void_v<DataType>, "DataType must be defined");
            static_assert(!std::is_void_v<PayloadPair>, "PayloadPair must be defined");
            static_assert(!std::is_void_v<IDPair>, "IDPair must be defined");
    };

/**
 * @brief Frame traits specialization for ConfigFrame.
 *
 * Provides traits for configuration frames with 20-byte total size
 * and 16-byte configuration payload. Used for setting baud rates,
 * filters, modes, and other adapter parameters.
 */
    template<> struct FrameTraits<ConfigFrame> {
        /// Total frame size in bytes
        static constexpr std::size_t FRAME_SIZE = 20;

        /// Bitmap size for configuration data
        static constexpr std::size_t BITMAP_SIZE = 4;

        // Alias for interface compatibility (ConfigFrame doesn't use ID/DATA pattern)
        static constexpr std::size_t ID_SIZE = 0; // Not applicable for config frames
        static constexpr std::size_t DATA_SIZE =
            0; // Not applicable for config frames

        /// Storage type for the entire frame (fixed-size array)
        using StorageType = std::array<std::byte, FRAME_SIZE>;

        /// Type for configuration data storage
        using FilterType = std::array<std::byte, BITMAP_SIZE>;
        using MaskType = std::array<std::byte, BITMAP_SIZE>;

        // Interface compatibility types (ConfigFrame has different semantics)
        using IDType = void; // Not applicable for config frames
        using DataType = void; // Not applicable for config frames
        using PayloadPair = void; // Not applicable for config frames
        using IDPair = void; // Not applicable for config frames

        private:
            // Compile-time interface validation
            static_assert(FRAME_SIZE == 20, "FRAME_SIZE must 20 bytes");
            static_assert(BITMAP_SIZE == 4, "CONFIG_SIZE must be 4 bytes");
            static_assert(!std::is_void_v<StorageType>, "StorageType must be defined");
            static_assert(!std::is_void_v<FilterType>, "FilterType must be defined");
            static_assert(!std::is_void_v<MaskType>, "MaskType must be defined");
    };

/**
 * @brief Helper alias template for accessing frame traits.
 *
 * Provides a convenient shortcut for accessing frame traits without
 * having to write the full FrameTraits<T> template instantiation.
 *
 * @tparam T Frame type (FixedFrame, VariableFrame, ConfigFrame)
 *
 * @example
 * @code
 * using Traits = frame_traits_t<FixedFrame>;
 * Traits::StorageType storage;
 * constexpr auto size = Traits::FRAME_SIZE;
 * @endcode
 */
    template<typename T> using frame_traits_t = FrameTraits<T>;

/**
 * @brief Compile-time constant for frame size lookup.
 *
 * Helper variable template that provides direct access to frame size
 * constants without having to instantiate the traits struct.
 * For VariableFrame, returns the maximum frame size.
 *
 * @tparam T Frame type
 *
 * @example
 * @code
 * static_assert(frame_size_v<FixedFrame> == 20);
 * static_assert(frame_size_v<VariableFrame> == 15);  // Max size
 * @endcode
 */
    template<typename T>
    inline constexpr std::size_t frame_size_v = []() {
            if constexpr (std::is_same_v<T, VariableFrame> ) {
                return FrameTraits<T>::MAX_FRAME_SIZE;
            } else {
                return FrameTraits<T>::FRAME_SIZE;
            }
        }();

/**
 * @brief Type trait to check if a type is a valid frame type.
 *
 * Provides compile-time checking to determine if a given type has
 * valid frame traits defined with all required interface members.
 * Useful for template constraints and SFINAE-based template selection.
 *
 * @tparam T Type to check
 */
    template<typename T, typename = void>
    struct is_frame_type : std::false_type {};

/**
 * @brief Specialization for valid frame types.
 *
 * A type is considered a valid frame type if it has FrameTraits
 * specialization with all required interface members and the
 * StorageType is not void (indicating proper specialization).
 */
    template<typename T>
    struct is_frame_type<T, std::void_t<typename FrameTraits<T>::StorageType,
        typename FrameTraits<T>::DataType,
        typename FrameTraits<T>::PayloadPair> >
        : std::bool_constant<
            !std::is_void_v<typename FrameTraits<T>::StorageType> > {};

/**
 * @brief Helper variable template for frame type checking.
 *
 * @tparam T Type to check
 *
 * @example
 * @code
 * static_assert(is_frame_type_v<FixedFrame>);
 * static_assert(!is_frame_type_v<int>);
 * @endcode
 */
    template<typename T>
    inline constexpr bool is_frame_type_v = is_frame_type<T>::value;

/**
 * @brief Concept-like validation for frame traits interface.
 *
 * Provides comprehensive compile-time validation that a frame type
 * implements the complete FrameTraits interface correctly.
 *
 * @tparam T Frame type to validate
 */
    template<typename T> struct validate_frame_traits {
        using Traits = FrameTraits<T>;

        // Validate required types are defined and not void
        static_assert(!std::is_void_v<typename Traits::StorageType>,
            "StorageType must be defined (not void)");
        static_assert(!std::is_void_v<typename Traits::DataType>,
            "DataType must be defined (not void)");
        static_assert(!std::is_void_v<typename Traits::PayloadPair>,
            "PayloadPair must be defined (not void)");

        // Validate PayloadPair structure
        static_assert(std::is_same_v<typename Traits::PayloadPair,
            std::pair<typename Traits::DataType, std::byte> >,
            "PayloadPair must be std::pair<DataType, byte>");

        // Frame-specific size validation
        static_assert(
            []() {
                if constexpr (std::is_same_v<T, VariableFrame> ) {
                    return Traits::MAX_FRAME_SIZE > 5 && Traits::MAX_FRAME_SIZE <= 15;
                } else {
                    return Traits::FRAME_SIZE == 20;
                }
            }(),
            "Frame size must be positive and 20 for fixed frames, from 5 to 15 for "
            "variable frames");

        static constexpr bool value = true;
    };

/**
 * @brief Helper variable template for frame traits validation.
 *
 * @tparam T Frame type to validate
 *
 * @example
 * @code
 * static_assert(is_valid_frame_traits_v<FixedFrame>);
 * @endcode
 */
    template<typename T>
    inline constexpr bool is_valid_frame_traits_v = validate_frame_traits<T>::value;

} // namespace USBCANBridge
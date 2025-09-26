#pragma once

#include "common.hpp"
#include "error.hpp"
#include "result.hpp"
#include "frame_traits.hpp"
#include <cstddef>
#include <cstdint>
#include <numeric>

namespace USBCANBridge {

    /**
     * @brief Checksum index mapping traits for different frame types.
     *
     * This template provides the checksum calculation range indices for each frame type.
     * It uses specialization to define the appropriate index ranges for each frame.
     */
    template<typename FrameType>
    struct ChksmTraits {
        // Primary template - intentionally empty to force specialization
    };

    /**
     * @brief Checksum index traits specialization for FixedFrame.
     */
    template<>
    struct ChksmTraits<FixedFrame> {
        static constexpr std::size_t START_INDEX = to_size_t(FixedSizeIndex::TYPE);
        static constexpr std::size_t END_INDEX = to_size_t(FixedSizeIndex::RESERVED);
        static constexpr std::size_t CHECKSUM_INDEX = to_size_t(FixedSizeIndex::CHECKSUM);
    };

    /**
     * @brief Checksum index traits specialization for ConfigFrame.
     */
    template<>
    struct ChksmTraits<ConfigFrame> {
        static constexpr std::size_t START_INDEX = to_size_t(ConfigCommandIndex::TYPE);
        static constexpr std::size_t END_INDEX = to_size_t(ConfigCommandIndex::BACKUP_3);
        static constexpr std::size_t CHECKSUM_INDEX = to_size_t(ConfigCommandIndex::CHECKSUM);
    };

    // Note: VariableFrame does not have a checksum, so no specialization is provided

    /**
     * @brief Utility class for frame checksum calculation and validation.
     *
     * This class provides polymorphic static methods that automatically determine
     * the correct checksum ranges based on the frame type using template specialization
     * and the FrameTraits system. This eliminates the need for frame-specific methods
     * while maintaining type safety and performance.
     */
    class ChksmUtil {
        public:
            /**
             * @brief Calculate checksum for any frame type using traits-based polymorphism.
             *
             * This method automatically determines the correct checksum range based on the
             * FrameType template parameter. It uses the frame's storage container and
             * leverages ChksmTraits to get the appropriate indices.
             *
             * @tparam FrameType The frame type (FixedFrame, ConfigFrame, etc.)
             * @param storage The frame's storage container
             * @return uint8_t The calculated checksum
             *
             * @example
             * @code
             * FixedFrame frame;
             * uint8_t checksum = ChksmUtil::calculateChecksum<FixedFrame>(frame.storage_);
             * @endcode
             */
            template<typename FrameType>
            static uint8_t calculateChecksum(
                const typename FrameTraits<FrameType>::StorageType& storage) {
                static_assert(is_frame_type_v<FrameType>, "FrameType must be a valid frame type");

                using IndexTraits = ChksmTraits<FrameType>;

                // Validate indices
                if (IndexTraits::START_INDEX > IndexTraits::END_INDEX ||
                    IndexTraits::END_INDEX >= storage.size()) {
                    return 0;
                }

                return std::transform_reduce(
                    storage.begin() + IndexTraits::START_INDEX,
                    storage.begin() + IndexTraits::END_INDEX + 1,
                    uint8_t{0},       // initial value
                    std::plus<uint8_t>{}, // reduction operation
                    [](std::byte b) { // transform operation
                        return static_cast<uint8_t>(b);
                    }
                );
            }

            /**
             * @brief Validate checksum for any frame type using traits-based polymorphism.
             *
             * This method automatically determines the correct checksum and validation ranges
             * based on the FrameType template parameter. It calculates the expected checksum
             * and compares it with the stored value.
             *
             * @tparam FrameType The frame type (FixedFrame, ConfigFrame, etc.)
             * @param storage The frame's storage container
             * @return Result<Status> Success if checksum is valid, error otherwise
             *
             * @example
             * @code
             * ConfigFrame frame;
             * auto result = ChksmUtil::validateChecksum<ConfigFrame>(frame.storage_);
             * if (result.success()) {
             *     // Checksum is valid
             * }
             * @endcode
             */
            template<typename FrameType>
            static Result<Status> validateChecksum(
                const typename FrameTraits<FrameType>::StorageType& storage) {
                static_assert(is_frame_type_v<FrameType>, "FrameType must be a valid frame type");

                using IndexTraits = ChksmTraits<FrameType>;

                // Validate container size
                if (IndexTraits::CHECKSUM_INDEX >= storage.size()) {
                    return Result<Status>::error(Status::WBAD_LENGTH);
                }

                // Calculate expected checksum
                uint8_t calculated = calculateChecksum<FrameType>(storage);

                // Get stored checksum from frame
                uint8_t stored = static_cast<uint8_t>(storage[IndexTraits::CHECKSUM_INDEX]);

                if (calculated != stored) {
                    return Result<Status>::error(Status::WBAD_CHECKSUM);
                }
                return Result<Status>::success(Status::SUCCESS);
            }

            // Legacy support methods for raw byte arrays (maintaining backward compatibility)

            /**
             * @brief Calculate checksum from raw byte stream with explicit indices.
             *
             * This method is provided for backward compatibility and cases where
             * raw byte arrays need to be processed without frame type information.
             *
             * @param data Pointer to the raw byte data
             * @param size Total size of the data buffer
             * @param start_idx Starting index for checksum calculation (inclusive)
             * @param end_idx Ending index for checksum calculation (inclusive)
             * @return uint8_t The calculated checksum
             */
            static uint8_t calculateChecksum(const std::byte* data, std::size_t size,
                std::size_t start_idx, std::size_t end_idx) {
                // Validate input parameters
                if (!data || size == 0 || start_idx > end_idx || end_idx >= size) {
                    return 0;
                }

                uint8_t sum = 0;
                for (std::size_t i = start_idx; i <= end_idx; ++i) {
                    sum += static_cast<uint8_t>(data[i]);
                }
                return sum;
            }

            /**
             * @brief Validate checksum from raw byte stream with explicit indices.
             *
             * This method is provided for backward compatibility and cases where
             * raw byte arrays need to be processed without frame type information.
             *
             * @param data Pointer to the raw byte data
             * @param size Total size of the data buffer
             * @param start_idx Starting index for checksum calculation (inclusive)
             * @param end_idx Ending index for checksum calculation (inclusive)
             * @param checksum_idx Index where the stored checksum is located
             * @return Result<Status> Success if checksum is valid, error otherwise
             */
            static Result<Status> validateChecksum(const std::byte* data, std::size_t size,
                std::size_t start_idx, std::size_t end_idx,
                std::size_t checksum_idx) {
                // Validate input parameters
                if (!data || size == 0 || checksum_idx >= size) {
                    return Result<Status>::error(Status::WBAD_LENGTH);
                }

                // Calculate expected checksum
                uint8_t calculated = calculateChecksum(data, size, start_idx, end_idx);

                // Get stored checksum from frame
                uint8_t stored = static_cast<uint8_t>(data[checksum_idx]);

                if (calculated != stored) {
                    return Result<Status>::error(Status::WBAD_CHECKSUM);
                }
                return Result<Status>::success(Status::SUCCESS);
            }
    };

} // namespace USBCANBridge
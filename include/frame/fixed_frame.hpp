#pragma once

#include "../interface/data.hpp"
#include "../interface/checksum.hpp"

namespace USBCANBridge {
    /**
     * @brief Fixed Frame implementation.
     *
     * This class represents a fixed frame in the USBCANBridge library.
     * It inherits from DataInterface and ChecksumInterface to provide
     * specific functionality for fixed-size (20 bytes) frames.
     *
     * Features:
     * - Fixed size with compile-time layout
     * - Automatic checksum calculation and validation
     * - Type-safe operations through interfaces
     *
     * @see File: include/protocol.hpp for constant definitions.
     * @note The frame structure is defined as follows:
     * ```
     * [START][HEADER][TYPE][FRAME_TYPE][FRAME_FMT][ID1][ID2][ID3][ID4][DLC][DATA0]...[DATA7][RESERVED][CHECKSUM]
     * ```
     * - `START` (1 byte): Start byte, always `0xAA`
     * - `HEADER` (1 byte): Header byte, always `0x55`
     * - `TYPE` (1 byte): Frame type, always `0x01`
     * - `FRAME_TYPE` (1 byte): Frame type,  `0x01` for Standard CAN ID, `0x02` for Extended CAN ID
     * - `FRAME_FMT` (1 byte): Frame format, always `0x01`
     * - `ID` (4 bytes): CAN ID, big-endian
     * - `DLC` (1 byte): Data Length Code, always `0x08`
     * - `DATA` (8 bytes): Data payload
     * - `RESERVED` (1 byte): Reserved byte, always `0x00`
     * - `CHECKSUM` (1 byte): Checksum byte, computed over bytes 2 to 18
     */

    class FixedFrame :
        public DataInterface<FixedFrame> {
        // * Alias for traits
        using traits = frame_traits_t<FixedFrame>;
        using layout = layout_t<FixedFrame>;
        using storage = storage_t<FixedFrame>;

        public:
            // * Constructors
            FixedFrame() : DataInterface<FixedFrame>() {
            }

            // === Core impl_*() Methods ===
            /**
             * @brief Initialize the frame fields.
             * This is called during construction to set up the frame.
             *
             * @see File: include/protocol.hpp for constant definitions.
             *
             * @see File: README.md for frame structure details.
             *
             * @note For a FixedFrame, the following fields are initialized:
             *
             * - `[START]` = `Constants::START_BYTE` (already set in CoreInterface)
             *
             * - `[HEADER]` = `Constants::HEADER`
             *
             * - `[TYPE]` = `Type::DATA_FIXED`
             *
             * - `[FRAME_TYPE]` = `FrameType::STD_FIXED`
             *
             * - `[FRAME_FMT]` = `FrameFmt::DATA_FIXED`
             *
             * - `[RESERVED]` = `Constants::RESERVED`
             */
            void impl_init_fields() {
                // * Set the Header byte
                frame_storage_[layout::HEADER_OFFSET] = to_byte(Constants::HEADER);
                // * Set the Type byte
                frame_storage_[layout::TYPE_OFFSET] = to_byte(Type::DATA_FIXED);
                // * Set the Frame Type byte
                frame_storage_[layout::FRAME_TYPE_OFFSET] = to_byte(FrameType::STD_FIXED);
                // * Set the Frame Format byte
                frame_storage_[layout::FORMAT_OFFSET] = to_byte(FrameFormat::DATA_FIXED);
                // * Set the Reserved byte
                frame_storage_[layout::RESERVED_OFFSET] = to_byte(Constants::RESERVED);
                return;
            }

            /**
             * @brief Get the size of the frame in bytes.
             * @return Result<std::size_t> The size of the frame in bytes.
             */
            Result<std::size_t> impl_size() const {
                return Result<std::size_t>::success(traits::FRAME_SIZE);
            }

            /**
             * @brief Deserialize the frame from a byte array.
             * This method copies the data into internal storage and performs complete validation before returning.
             * @param data A span representing the serialized frame data.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             * @note This method copies the data into internal storage and marks the checksum as dirty.
             */
            Result<void> impl_deserialize(span<const std::byte> data);

            // === DataFrame impl_*() Methods ===
            /**
             * @brief Get the frame format from the internal storage.
             * @return Result<FrameFormat>
             */
            Result<FrameFormat> impl_get_format() const;
            /**
             * @brief Set the frame format in the internal storage.
             * @warning Changing the format marks the frame as dirty, requiring checksum recomputation.
             * @param format The FrameFormat to set. Must be one of the valid enum values.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             */
            Result<void> impl_set_format(FrameFormat format);
            /**
             * @brief Get the frame ID from the internal storage.
             * @return Result<uint32_t>
             */
            Result<uint32_t> impl_get_id() const;
            /**
             * @brief Set the frame ID in the internal storage.
             * @warning Changing the ID marks the frame as dirty, requiring checksum recomputation.
             * @param id The frame ID to set.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             */
            Result<void> impl_set_id(uint32_t id);
            /**
             * @brief Get the data length code (DLC) from the internal storage.
             * @return Result<std::size_t>
             */
            Result<std::size_t> impl_get_dlc() const;
            /**
             * @brief Get a read-only view of the data payload.
             * @return Result<span<const std::byte>>
             */
            Result<span<const std::byte> > impl_get_data() const;
            /**
             * @brief Set the data payload in the internal storage.
             * @warning Changing the data marks the frame as dirty, requiring checksum recomputation.
             * @param data A span representing the data payload to set.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             */
            Result<void> impl_set_data(span<const std::byte> data);
            /**
             * @brief Check if the frame is using an extended CAN ID.
             * @return Result<bool>
             */
            Result<bool> impl_is_extended() const;

        private:
            /**
             * @brief Set the data length code (DLC) in the internal storage.
             * @warning Changing the DLC marks the frame as dirty, requiring checksum recomputation.
             * @param dlc The DLC to set.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             */
            Result<void> impl_set_dlc(std::size_t dlc);

        public:
            // === DataValidator impl_*() Methods ===
            /**
             * @brief Retrieve the header byte from internal storage.
             * @return Result<uint8_t> The header byte, or an error status on failure.
             */
            Result<uint8_t> impl_get_header_byte() const {
                return Result<uint8_t>::success(
                    static_cast<uint8_t>(frame_storage_[layout::HEADER_OFFSET]));
            }

            // === CoreValidator impl_*() Methods ===
            /**
             * @brief Validate the overall structure of the frame.
             * This includes checking constant fields like START, HEADER, TYPE, FRAME_TYPE, FORMAT, and RESERVED.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             */
            Result<bool> impl_validate() const;

    };
}
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
     * [START][HEADER][TYPE][FRAME_TYPE][FORMAT][ID1][ID2][ID3][ID4][DLC][DATA0]...[DATA7][RESERVED][CHECKSUM]
     * ```
     * - `START` (1 byte): Start byte, always `0xAA`
     * - `HEADER` (1 byte): Header byte, always `0x55`
     * - `TYPE` (1 byte): Frame type, always `0x01`
     * - `FRAME_TYPE` (1 byte): Frame type,  `0x01` for Standard CAN ID, `0x02` for Extended CAN ID
     * - `FORMAT` (1 byte): Frame format, always `0x01`
     * - `ID` (4 bytes): CAN ID, big-endian
     * - `DLC` (1 byte): Data Length Code, always `0x08`
     * - `DATA` (8 bytes): Data payload
     * - `RESERVED` (1 byte): Reserved byte, always `0x00`
     * - `CHECKSUM` (1 byte): Checksum byte, computed over bytes 2 to 18
     */

    class FixedFrame :
        public DataInterface<FixedFrame> {
        private:
            // * Composition with ChecksumInterface
            ChecksumInterface<FixedFrame> checksum_interface_;

        public:
            // * Constructors
            FixedFrame() : DataInterface<FixedFrame>(), checksum_interface_(*this) {
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
             * - `[FRAME_TYPE]` = `CANVersion::STD_FIXED`
             *
             * - `[FORMAT]` = `Format::DATA_FIXED`
             *
             * - `[RESERVED]` = `Constants::RESERVED`
             */
            void impl_init_fields() {
                // * Set the Header byte
                frame_storage_[layout_.HEADER] = to_byte(Constants::HEADER);
                // * Set the Type byte
                frame_storage_[layout_.TYPE] = to_byte(Type::DATA_FIXED);
                // * Set the Frame Type byte
                frame_storage_[layout_.CANVERS] = to_byte(CANVersion::STD_FIXED);
                // * Set the Frame Format byte
                frame_storage_[layout_.FORMAT] = to_byte(Format::DATA_FIXED);
                // * Set the Reserved byte
                frame_storage_[layout_.RESERVED] = to_byte(Constants::RESERVED);
                // Mark checksum as dirty since we changed the frame
                checksum_interface_.mark_dirty();
            }

            /**
             * @brief Get the size of the frame in bytes.
             * @return std::size_t The size of the frame in bytes.
             */
            std::size_t impl_size() const {
                return traits_.FRAME_SIZE;
            }
            /**
             * @brief Get the frame Type byte.
             * The Type byte indicates the frame type and format.
             * @return std::byte The Type byte.
             */
            std::byte impl_get_type() const {
                return frame_storage_[layout_.TYPE];
            }
            /**
             * @brief Set the frame Type byte.
             * @warning Changing the type marks the frame as dirty, requiring checksum recomputation.
             * @param type The Type byte to set. Must be one of the valid enum values.
             */
            void impl_set_type(Type type) {
                frame_storage_[layout_.TYPE] = to_byte(type);
                // Mark checksum as dirty since we changed the frame
                checksum_interface_.mark_dirty();
            }

            // === DataFrame impl_*() Methods ===
            /**
             * @brief Get the version of the CAN ID (standard/extended).
             * @return CANVersion The CANVersion byte of the frame.
             */
            CANVersion impl_get_CAN_version() const;

            /**
             * @brief Set the version of the CAN ID (standard/extended).
             *
             */
            void impl_set_CAN_version(CANVersion ver);

            /**
             * @brief Get the frame format from the internal storage.
             * @return Format The frame format
             */
            Format impl_get_format() const;
            /**
             * @brief Set the frame format in the internal storage.
             * @warning Changing the format marks the frame as dirty, requiring checksum recomputation.
             * @param format The Format to set. Must be one of the valid enum values.
             */
            void impl_set_format(Format format);
            /**
             * @brief Get the frame ID from the internal storage.
             * @note The ID is stored in little-endian format.
             * @return uint32_t The CAN ID
             */
            uint32_t impl_get_CAN_id() const;
            /**
             * @brief Set the frame ID in the internal storage.
             * @note The ID is stored in little-endian format.
             * @warning Changing the ID marks the frame as dirty, requiring checksum recomputation.
             * @param id The frame ID to set.
             */
            void impl_set_CAN_id(uint32_t id);
            /**
             * @brief Get the data length code (DLC) from the internal storage.
             * @return std::size_t The DLC value
             */
            std::size_t impl_get_dlc() const;
            /**
             * @brief Get a read-only view of the data payload.
             * @return span<const std::byte> View of the data
             */
            span<const std::byte> impl_get_data() const;
            /**
             * @brief Set the data payload in the internal storage.
             * @warning Changing the data marks the frame as dirty, requiring checksum recomputation.
             * @param data A span representing the data payload to set.
             */
            void impl_set_data(span<const std::byte> data);
            /**
             * @brief Check if the frame is using an extended CAN ID.
             * @return bool True if extended, false if standard
             */
            bool impl_is_extended() const;

        private:
            /**
             * @brief Set the data length code (DLC) in the internal storage.
             * @warning Changing the DLC marks the frame as dirty, requiring checksum recomputation.
             * @param dlc The DLC to set.
             */
            void impl_set_dlc(std::size_t dlc);

    };
}
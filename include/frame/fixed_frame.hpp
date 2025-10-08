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
            // * Internal state variables
            Format current_format_;
            CANVersion current_version_;
            std::size_t current_dlc_ = 0; // Data Length Code (0-8)
            std::array<std::byte, 4> init_id; // Initial ID bytes (2 or 4 bytes, little-endian)
            std::array<std::byte, 8> init_data; // Initial data bytes (0-8 bytes)
            // * Composition with ChecksumInterface
            ChecksumInterface<FixedFrame> checksum_interface_;

        public:
            // * Constructors
            FixedFrame() : FixedFrame(Format::DATA_FIXED, CANVersion::STD_FIXED, {}, 0, {}) {
            }
            FixedFrame(Format fmt, CANVersion ver, std::array<std::byte, 4> init_id = {},
                std::size_t init_dlc = 0,
                std::array<std::byte, 8> init_data = {}) : DataInterface<FixedFrame>(),
                current_format_(fmt),
                current_version_(ver),
                current_dlc_(init_dlc),
                init_id(init_id),
                init_data(init_data),
                checksum_interface_(*this) {

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
                frame_storage_[layout_.CAN_VERS] = to_byte(CANVersion::STD_FIXED);
                // * Set the Frame Format byte
                frame_storage_[layout_.FORMAT] = to_byte(Format::DATA_FIXED);
                // * Set the Reserved byte
                frame_storage_[layout_.RESERVED] = to_byte(Constants::RESERVED);
                // # if we have an initial ID, set it
                if (!init_id.empty()) {
                    std::copy(init_id.begin(), init_id.end(), frame_storage_.begin() + layout_.ID);
                }
                // # if we have an initial data payload, set it
                if (!init_data.empty()) {
                    std::copy(init_data.begin(), init_data.end(),
                        frame_storage_.begin() + layout_.DATA);
                    // Set the DLC accordingly
                    current_dlc_ = std::min(init_data.size(), static_cast<std::size_t>(8));
                    frame_storage_[layout_.DLC] = static_cast<std::byte>(current_dlc_);
                } else {
                    // Ensure DLC is zero if no data
                    current_dlc_ = 0;
                    frame_storage_[layout_.DLC] = static_cast<std::byte>(0);
                }
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
            // === Finalization ===
            /**
             * @brief Finalize the frame before transmission.
             * This method ensures the frame is ready to be sent, including checksum calculation.
             * @note This calls ChecksumInterface::finalize() to compute and set the checksum.
             */
            void finalize() {
                checksum_interface_.update_checksum();
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
} // namespace USBCANBridge

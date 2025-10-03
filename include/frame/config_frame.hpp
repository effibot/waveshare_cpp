#pragma once

#include "../interface/config.hpp"
#include "../interface/checksum.hpp"



namespace USBCANBridge {
    /**
     * @brief Configuration frame for the USB-CAN Bridge.
     *
     * This class represents the configuration frame used by the USB-CAN Bridge.
     * It provides methods to access and modify the configuration parameters.
     * It inherits from `ConfigInterface` to provide a clean interface for configuration operations.
     * It also inherits from `ChecksumInterface` to handle checksum calculation and validation.
     * The frame has a fixed size of 20 bytes and a specific layout as defined in the protocol.
     * @see File: include/protocol.hpp for constant definitions.
     * @note The frame structure is defined as follows:
     * ```
     * [START][HEADER][TYPE][CAN_BAUD][FRAME_TYPE][FILTER_ID0-3][MASK_ID0-3][CAN_MODE][AUTO_RTX][RESERVED0-3][CHECKSUM]
     * ```
     * - `START` (1 byte): Start byte, always `0xAA`
     * - `HEADER` (1 byte): Header byte, always `0x55`
     * - `TYPE` (1 byte): Frame type, always `0x02`
     * - `CAN_BAUD` (1 byte): CAN baud rate
     * - `CAN_MODE` (1 byte): CAN mode
     * - `FILTER` (4 bytes): Acceptance filter, big-endian
     * - `MASK` (4 bytes): Acceptance mask, big-endian
     * - `AUTO_RTX` (1 byte): Auto retransmission setting
     * - `RESERVED` (4 bytes): Reserved bytes, always `0x00`
     * - `CHECKSUM` (1 byte): Checksum byte, computed over bytes 2 to 14
     *
     * @see File: README.md for frame structure details.
     */
    class ConfigFrame :
        public ConfigInterface<ConfigFrame>,
        public ChecksumInterface<ConfigFrame> {
        // * Alias for traits
        using traits = frame_traits_t<ConfigFrame>;
        using layout = layout_t<ConfigFrame>;
        using storage = storage_t<ConfigFrame>;

        public:
            // * Constructors
            ConfigFrame() : ConfigInterface<ConfigFrame>(), ChecksumInterface<ConfigFrame>() {
                // Initialize constant fields
                impl_init_fields();
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
             * @note For a ConfigFrame, the following fields are initialized:
             * ```
             * [START][HEADER][TYPE][BAUD][MODE][FILTER1][FILTER2][FILTER3][FILTER4][MASK1][MASK2][MASK3][MASK4][RESERVED1][RESERVED2]
             * ```
             * - `START` (1 byte): Start byte, always `0xAA` (already set in CoreInterface)
             * - `HEADER` (1 byte): Header byte, always `0x55`
             * - `TYPE` (1 byte): Frame type, always `0x02`
             * - `BAUD` (1 byte): CAN baud rate
             * - `MODE` (1 byte): CAN mode
             * - `FILTER` (4 bytes): Acceptance filter, big-endian
             * - `MASK` (4 bytes): Acceptance mask, big-endian
             * - `RESERVED` (4 bytes): Reserved bytes, always `0x00`
             */
            Result<void> impl_init_fields() {
                // * Set the Header byte
                frame_storage_[layout::HEADER_OFFSET] = to_byte(Constants::HEADER);
                // * Set the Type byte
                frame_storage_[layout::TYPE_OFFSET] = to_byte(DEFAULT_CONF_TYPE);
                // * Initialize other fields to zero
                frame_storage_[layout::BAUD_OFFSET] = to_byte(CANBaud::BAUD_1M);
                frame_storage_[layout::MODE_OFFSET] = to_byte(CANMode::NORMAL);
                for (size_t i = 0; i < 4; ++i) {
                    frame_storage_[layout::FILTER_OFFSET + i] = to_byte(Constants::RESERVED);
                    frame_storage_[layout::MASK_OFFSET + i] = to_byte(Constants::RESERVED);
                    frame_storage_[layout::RESERVED_OFFSET + i] = to_byte(Constants::RESERVED);
                }
                // Mark checksum as dirty since we changed the frame
                this->mark_dirty();
                return Result<void>::success();
            }

            /**
             * @brief Deserialize the frame from a byte array.
             * This method copies the data into internal storage and performs complete validation before returning.
             * @param data A span representing the serialized frame data.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             * @note This method copies the data into internal storage and marks the checksum as dirty.
             */
            Result<void> impl_deserialize(span<const std::byte> data);
            /**
             * @brief Validate the frame contents.
             * This method checks the integrity and correctness of the frame fields.
             * @note A given buffer is a config frame if:
             *
             * - It has the correct fixed size (20 bytes).
             *
             * - The `START` byte is `Constants::START_BYTE`.
             *
             * - The `HEADER` byte is `Constants::HEADER`.
             *
             * - The `TYPE` byte is either `Type::CONF_FIXED` or `Type::CONF_VARIABLE`.
             *
             * - The `CAN_BAUD` byte is one of the supported baud rates.
             *
             * - The `CAN_MODE` byte is one of the supported CAN modes.
             *
             * - The `FILTER` and `MASK` values are compatible with the selected frame type.
             *
             * - The `RESERVED` bytes are all `Constants::RESERVED`.
             *
             * - The `CHECKSUM` byte matches the computed checksum.
             *
             * @see File: include/protocol.hpp for constant definitions.
             * @see File: README.md for frame structure details.
             * @see ChecksumInterface::compute_checksum() for checksum calculation.
             *
             * @param data The buffer to validate.
             * @return Result<bool> indicating whether the frame is valid.
             */
            Result<bool> impl_validate(span<const std::byte> data) const;
            /**
             * @brief Get the size of the frame in bytes.
             *
             * @return Result<std::size_t> The size of the frame in bytes.
             */
            Result<std::size_t> impl_size() const {
                return Result<std::size_t>::success(traits::FRAME_SIZE);
            }




            // === ConfigFrame impl_*() Methods ===
            /**
             * @brief Get the CAN baud rate from the internal storage.
             * @return Result<CANBaud> The CAN baud rate, or an error status on failure.
             */
            Result<CANBaud> impl_get_baud_rate() const;
            /**
             * @brief Set the CAN baud rate in the internal storage.
             * @warning Changing the baud rate marks the frame as dirty, requiring checksum recomputation.
             * @param rate The CAN baud rate to set.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             */
            Result<void> impl_set_baud_rate(CANBaud rate);

            /**
             * @brief Get the CAN mode from the internal storage.
             * @return Result<CANMode> The CAN mode, or an error status on failure.
             */
            Result<CANMode> impl_get_can_mode() const;
            /**
             * @brief Set the CAN mode in the internal storage.
             * @warning Changing the mode marks the frame as dirty, requiring checksum recomputation.
             * @param mode The CAN mode to set.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             */
            Result<void> impl_set_can_mode(CANMode mode);
            /**
             * @brief Get the acceptance filter from the internal storage.
             * @return Result<uint32_t> The acceptance filter, or an error status on failure.
             */
            Result<uint32_t> impl_get_filter() const;
            /**
             * @brief Set the acceptance filter in the internal storage.
             * @warning Changing the filter marks the frame as dirty, requiring checksum recomputation.
             * @param filter The acceptance filter to set.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             * @note The filter must be compatible with the current frame type.
             */
            Result<void> impl_set_filter(uint32_t filter);
            /**
             * @brief Get the acceptance mask from the internal storage.
             * @return Result<uint32_t> The acceptance mask, or an error status on failure.
             */
            Result<uint32_t> impl_get_mask() const;
            /**
             * @brief Set the acceptance mask in the internal storage.
             * @warning Changing the mask marks the frame as dirty, requiring checksum recomputation.
             * @param mask The acceptance mask to set.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure
             * @note The mask must be compatible with the current filter and frame type.
             */
            Result<void> impl_set_mask(uint32_t mask);
            /**
             * @brief Set the auto retransmission flag in the internal storage.
             * @param enable True to enable auto retransmission, false to disable it.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             */
            Result<void> impl_set_auto_rtx(RTX auto_rtx);
            /**
             * @brief Get the auto retransmission flag from the internal storage.
             * @return Result<RTX> The current auto retransmission setting, or an error status on failure.
             */
            Result<RTX> impl_get_auto_rtx() const;
    };
}
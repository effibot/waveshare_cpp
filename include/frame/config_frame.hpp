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
        public ConfigInterface<ConfigFrame> {

        private:
            // * Internal state variables (if any)
            Type init_type_;
            std::array<std::byte, 4> init_filter_;
            std::array<std::byte, 4> init_mask_;
            RTX init_auto_rtx_;
            CANBaud init_baud_;
            CANMode init_mode_;
            // * Composition with ChecksumInterface
            ChecksumInterface<ConfigFrame> checksum_interface_;

        public:
            // * Constructors
            ConfigFrame(
                Type type = DEFAULT_CONF_TYPE,
                std::array<std::byte, 4> filter = {},
                std::array<std::byte, 4> mask = {},
                RTX auto_rtx = RTX::AUTO,
                CANBaud baud = CANBaud::BAUD_1M,
                CANMode mode = CANMode::NORMAL
            ) : ConfigInterface<ConfigFrame>(),
                init_type_(type),
                init_filter_(filter),
                init_mask_(mask),
                init_auto_rtx_(auto_rtx),
                init_baud_(baud),
                init_mode_(mode),
                checksum_interface_(*this) {
                // * The base constructor will call impl_init_fields(), here defined.
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
            void impl_init_fields() {
                // * Set the Header byte
                frame_storage_[layout_.HEADER] = to_byte(Constants::HEADER);
                // * Set the Type byte
                frame_storage_[layout_.TYPE] = to_byte(init_type_);
                // * Initialize other fields to zero
                frame_storage_[layout_.BAUD] = to_byte(init_baud_);
                frame_storage_[layout_.MODE] = to_byte(init_mode_);
                for (size_t i = 0; i < 4; ++i) {
                    frame_storage_[layout_.FILTER + i] = to_byte(init_filter_[i]);
                    frame_storage_[layout_.MASK + i] = to_byte(init_mask_[i]);
                    frame_storage_[layout_.RESERVED + i] = to_byte(Constants::RESERVED);
                }
                // * Set the Auto Retransmission byte
                frame_storage_[layout_.AUTO_RTX] = to_byte(init_auto_rtx_);
                // Mark checksum as dirty since we changed the frame
                checksum_interface_.mark_dirty();
            }

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
            //Result<bool> impl_validate(span<const std::byte> data) const;
            /**
             * @brief Get the size of the frame in bytes.
             *
             * @return std::size_t The size of the frame in bytes.
             */
            std::size_t impl_size() const {
                return traits_.FRAME_SIZE;
            }

            // === ConfigFrame impl_*() Methods ===
            /**
             * @brief Get the type of the frame.
             *
             * @return Type The type of the frame.
             */
            Type impl_get_type() const;
            /**
             * @brief Set the type of the frame.
             *
             * @param type The type to set.
             */
            void impl_set_type(Type type);
            /**
             * @brief Get the CAN version of the frame.
             *
             * @return CANVersion The CAN version of the frame.
             */
            CANVersion impl_get_CAN_version() const;
            /**
             * @brief Set the CAN version of the frame.
             *
             * @param type The CAN version to set.
             */
            void impl_set_CAN_version(CANVersion type);

            // === Finalization ===
            /**
             * @brief Finalize the frame before transmission.
             * This method ensures the frame is ready to be sent, including checksum calculation.
             * @note This calls ChecksumInterface::finalize() to compute and set the checksum.
             */
            void finalize() {
                checksum_interface_.update_checksum();
            }
    };
}
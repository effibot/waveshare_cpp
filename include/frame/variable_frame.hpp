#pragma once

#include "../interface/data.hpp"

namespace USBCANBridge {
    /**
     * @brief Variable frame implementation.
     *
     * This class represents a variable frame in the USBCANBridge library.
     * It inherits from BaseFrame and provides specific functionality for
     * variable-size frames (5-15 bytes) with dynamic CAN ID and data payload.
     *
     * Features:
     * - Variable size (5-15 bytes) with dynamic allocation
     * - Support for standard (11-bit) and extended (29-bit) CAN IDs
     * - Variable data payload (0-8 bytes)
     * - Type-safe operations through BaseFrame interface
     * - No checksum - uses END byte terminator
     *
     * Structure: [START][TYPE][ID0-ID3][DATA0-DATA7][END]
     */
    class VariableFrame : public DataInterface<VariableFrame> {
        // * Alias for traits
        using traits = frame_traits_t<VariableFrame>;
        using layout = layout_t<VariableFrame>;
        using storage = storage_t<VariableFrame>;


        public:
            // * Constructors
            VariableFrame() : DataInterface<VariableFrame>() {
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
             * @note For a VariableFrame, the following fields are initialized:
             *
             * - `START` (1 byte): Start byte, always `0xAA` (already set in CoreInterface)
             * - `TYPE` (1 byte): Frame type, always `0xC0` for variable frames
             * - `ID` (4 bytes): CAN ID, little-endian, initialized to 0
             * - `DATA` (0-8 bytes): CAN payload, initialized as empty.
             * - `END` (1 byte): End byte, always `0x55`
             */
            void impl_init_fields() {
                // * Set the Type byte to default variable frame type
                frame_storage_[layout::TYPE] = to_byte(Type::DATA_VARIABLE);
                // * Initialize ID to 0
                frame_storage_[layout::ID] = to_byte(Constants::RESERVED);
                // * Compute the initial size with 0 data length
                std::size_t initial_size = layout::frame_size(false, 0);
                // * Set the END byte at the correct position
                frame_storage_[initial_size - 1] = to_byte(Constants::END_BYTE);
            }

            /**
             * @brief Get the current size of the frame in bytes.
             * This includes all fields: START, TYPE, ID, DATA, END.
             * The size is dynamic based on the current DLC (data length code).
             *
             * @return std::size_t The current size of the frame in bytes.
             */
            std::size_t impl_size() const;

            /**
             * @brief Validate the frame.
             * This checks that all fields are valid according to the protocol.
             *
             * @return bool True if valid, false if invalid.
             */
            bool impl_validate() const;


            /**
             * @brief Get the Type byte from the frame storage.
             *
             * @return std::byte The Type byte.
             */
            std::byte impl_get_type() const;

            /**
             * @brief Set the Type byte in the frame storage.
             *
             * @param type The Type byte to set.
             */
            void impl_set_type(std::byte type);

            /**
             * @brief Deserialize the frame from a byte array.
             * This method copies the data into internal storage and performs complete validation before returning.
             * @param data A span representing the serialized frame data.
             * @note This method copies the data into internal storage and marks the checksum as dirty.
             */
            void impl_deserialize(span<const std::byte> data);

        // === DataFrame impl_*() Methods ===
        public:
            /**
             * @brief Get the frame format from the internal storage.
             * @return Format The frame format
             */
            Format impl_get_format() const;

            /**
             * @brief Set the Frame Format object.
             * @param format The Format to set. Must be one of the valid enum values.
             */
            void impl_set_format(Format format);
            /**
             * @brief Get the frame ID from the internal storage.
             * @return uint32_t The CAN ID
             */
            uint32_t impl_get_id() const;
            /**
             * @brief Set the frame ID in the internal storage.
             * @warning Changing the ID marks the frame as dirty, requiring checksum recomputation.
             * @param id The frame ID to set.
             */
            void impl_set_id(uint32_t id);
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
             * @brief Set the DLC field in the Type byte.
             * @param dlc The data length code (DLC) to set.
             */
            void impl_set_dlc(std::size_t dlc);

    };
}
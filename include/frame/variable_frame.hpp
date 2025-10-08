#pragma once

#include "../interface/data.hpp"
#include "../interface/vartype.hpp"

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
     * - Automatic Type field reconstruction via VarTypeInterface
     * - Cached Format, CANVersion, and DLC for efficient manipulation, without bitwise ops. Type field consistency is ensured via VarTypeInterface and the dirty bit mechanism.
     * Structure: [START][TYPE][ID0-ID3][DATA0-DATA7][END]
     */
    class VariableFrame : public DataInterface<VariableFrame> {
        /**
         * From FrameTraits<VariableFrame>, we have:
         * - pre-allocated buffer of 5 bytes (START, TYPE, ID0, ID1, END), w/o payload
         * - layout structure with helpers for dynamic offsets and sizes
         * - same storage type (span<std::byte>) as other frames, but dynamic size
         */

        private:
            // * Internal state variables
            Format current_format_;
            CANVersion current_version_;
            std::size_t current_dlc_ = 0; // Data Length Code (0-8)
            std::vector<std::byte> init_id; // Initial ID bytes (2 or 4 bytes, little-endian)
            std::vector<std::byte> init_data; // Initial data bytes (0-8 bytes)
            // * Helper interface for Type field manipulation
            VarTypeInterface<VariableFrame> var_type_interface_;

            // # Utility to split and combine the buffer when size changes

            /**
             * @brief Utility to split the internal buffer up to a given offset.
             * This will generate two new buffers, allocating new memory as needed.
             * The first buffer will contain the data up to the given offset (included), the second buffer will contain the rest.
             * @param offset The offset to split at (must be <= current size)
             * @throw std::out_of_range if the offset is out of bounds.
             * @note This will not update the internal state of the frame and should be used alongside the `merge_buffer()` method to reconstruct a valid buffer and update the frame.
             * @return A pair of vectors containing the two parts of the split buffer.
             */
            std::pair<std::vector<std::byte>, std::vector<std::byte> >
            split_buffer(std::size_t offset) const {
                if (offset > impl_size()) {
                    throw std::out_of_range("Offset out of bounds in split_buffer");
                }
                auto full_span = this->get_storage();
                std::vector<std::byte> first_part(full_span.begin(), full_span.begin() + offset);
                std::vector<std::byte> second_part(full_span.begin() + offset, full_span.end());
                return {first_part, second_part};
            }
            /**
             * @brief Utility to merge two buffers into the internal storage.
             * This will concatenate the two buffers and update the internal storage.
             * @param first The first buffer part.
             * @param second The second buffer part.
             * @throw std::invalid_argument if the resulting size is out of bounds (5-15 bytes).
             * @note This will update the internal state of the frame to reflect the new size.
             */
            void merge_buffer(const std::vector<std::byte>& first,
                const std::vector<std::byte>& second) {
                std::size_t new_size = first.size() + second.size();
                if (new_size < traits_.MIN_FRAME_SIZE || new_size > traits_.MAX_FRAME_SIZE) {
                    throw std::invalid_argument(
                        "Resulting buffer size out of bounds in merge_buffer");
                }
                // Create new buffer and copy data
                std::vector<std::byte> new_buffer;
                new_buffer.reserve(new_size);
                new_buffer.insert(new_buffer.end(), first.begin(), first.end());
                new_buffer.insert(new_buffer.end(), second.begin(), second.end());
                // Update the frame storage to the new buffer
                traits_.frame_buffer = std::move(new_buffer);
                frame_storage_ = span<std::byte>(traits_.frame_buffer.data(), new_size);
                // Mark Type field as dirty to ensure it's recomputed before sending
                var_type_interface_.mark_dirty();
            }
        public:
            // * Constructors
            VariableFrame(Format fmt = Format::DATA_VARIABLE,
                CANVersion ver = CANVersion::STD_VARIABLE,
                std::vector<std::byte> init_id = {},
                size_t payload_size = 0,
                std::vector<std::byte> init_data = {}) : DataInterface<VariableFrame>(),
                current_format_(fmt),
                current_version_(ver),
                current_dlc_(payload_size),
                init_id(std::move(init_id)),
                init_data(std::move(init_data)),
                var_type_interface_(*this) {
                // * The base constructor will call impl_init_fields(), here defined.
            }

            VariableFrame() : VariableFrame(Format::DATA_VARIABLE,
                    CANVersion::STD_VARIABLE, {}, 0, {}) {
            }

            // === Core impl_*() Methods ===
            /**
             * @brief Initialize the frame fields.
             * This is called during construction to set up the frame.
             * @see File: include/protocol.hpp for constant definitions.
             * @see File: README.md for frame structure details.
             * @note For a VariableFrame, the following fields are initialized:
             * ```
             * [START][TYPE][ID0-ID1][END]
             * ```
             * - `[START]` = `Constants::START_BYTE` (already set in CoreInterface)
             * - `[TYPE]` = `Type::DATA_VARIABLE`
             * - `[END]` = `Constants::END_BYTE`
             */
            void impl_init_fields() {
                bool must_resize = false;
                std::size_t new_size = traits_.MIN_FRAME_SIZE; // Minimum size (5 bytes)
                // # if we have Extended ID, resize the buffer during allocation, since only the START byte has been set so far
                if (current_version_ == CANVersion::EXT_VARIABLE) {
                    // resize the buffer to 6 bytes (START, TYPE, ID0, ID1, ID2, ID3, END)
                    new_size += 2; // +2 bytes for extended ID
                    must_resize = true;
                }
                // # if we already know the payload size, resize the buffer during allocation
                if (current_dlc_ > 0) {
                    if (current_dlc_ > traits_.MAX_DATA_SIZE) {
                        throw std::out_of_range("Payload size exceeds maximum for VariableFrame");
                    }
                    // Resize the buffer to accommodate the payload
                    new_size += current_dlc_; // +N bytes for data payload
                    must_resize = true;
                }
                if (must_resize) {
                    traits_.frame_buffer.resize(new_size);
                    frame_storage_ = span<std::byte>(traits_.frame_buffer.data(), new_size);
                }
                // * Set the Type byte
                frame_storage_[layout_.TYPE] = to_byte(Type::DATA_VARIABLE);
                // * Set the END byte
                frame_storage_[layout_.end(impl_is_extended(),
                    0) - 1] = to_byte(Constants::END_BYTE);
                // * If we have an initial ID, set it
                if (!init_id.empty()) {
                    auto id_span = this->get_storage().subspan(
                        layout_.ID, init_id.size());
                    std::copy(init_id.begin(), init_id.end(), id_span.begin());
                }
                // * If we have an initial data payload, set it
                if (!init_data.empty()) {
                    if (init_data.size() > traits_.MAX_DATA_SIZE ||
                        init_data.size() != current_dlc_) { // <<< check that the user provided a consistent DLC
                        throw std::out_of_range(
                            "Initial data size exceeds maximum for VariableFrame");
                    }
                    auto data_span = this->get_storage().subspan(
                        layout_.data(impl_is_extended()), init_data.size());
                    std::copy(init_data.begin(), init_data.end(), data_span.begin());


                }

                // * Mark the Type field as dirty to ensure it's computed before sending
                var_type_interface_.mark_dirty();
                return;
            }

            /**
             * @brief Get the size of the frame in bytes.
             * This is dynamic, based on current ID and data sizes.
             * @return std::size_t The size of the frame in bytes.
             */
            std::size_t impl_size() const {
                bool extended = impl_is_extended();
                std::size_t data_len = impl_get_dlc();
                return layout_.frame_size(extended, data_len);
            }

            /**
             * @brief Get the frame Type byte.
             * The Type byte indicates the CAN version (standard/extended), the format (data/remote), and the length of the data payload (DLC).
             * @return `Type` The Type byte.
             * @note The Type byte is composed of:
             *
             * - Bits 7-6: constant Type byte for variable frames (`0xC0`)
             *
             * - Bit 5: CAN version (0 = standard, 1 = extended)
             *
             * - Bit 4: Frame format (0 = data frame, 1 = remote frame)
             *
             * - Bits 3-0: Data Length Code (DLC), number of data
             *
             * ! This gets a computed Type byte from the cached values using VarTypeInterface
             */
            // ! Check this cast as the content of the TYPE byte is not a valid enum value but the sum of each sub-field
            Type impl_get_type() const;

            /**
             * @brief Set the frame Type byte.
             * The Type byte indicates the CAN version (standard/extended), the format (data/remote), and the length of the data payload (DLC).
             * @warning Changing the type marks the frame as dirty, requiring checksum recomputation.
             * @note As the DLC is automatically managed when setting data, this method does not allow setting it directly. Instead, it is derived from the actual data length.
             * @param ver The CAN version (standard/extended)
             * @param fmt The frame format (data/remote)
             * @note The Type byte is composed of:
             *
             * - Bits 7-6: constant Type byte for variable frames (`0xC0`)
             *
             * - Bit 5: CAN version (0 = standard, 1 = extended)
             *
             * - Bit 4: Frame format (0 = data frame, 1 = remote frame)
             *
             * - Bits 3-0: Data Length Code (DLC), number of data
             *
             * This means that the provided value is not a valid enum but is casted to `Type` for convenience.
             */
            void impl_set_type(CANVersion ver, Format fmt);

            void impl_finalize() {
                // Ensure the Type field is up to date
                var_type_interface_.update_type(
                    current_version_, current_format_, current_dlc_
                );
                return;
            }

            // === DataFrame impl_*() Methods ===
            /**
             * @brief Check if the frame is using an extended CAN ID.
             * @return true if extended (29-bit), false if standard (11-bit)
             * @note This uses the cached current_version_ variable for efficiency.
             */
            bool impl_is_extended() const;


        private:
            /**
             * @brief Set the data length code (DLC) for the frame.
             * @param dlc The DLC value to set (0-8).
             * @throw std::out_of_range if the DLC is out of bounds.
             * @warning Changing the DLC marks the frame as dirty, requiring type recomputation when needed.
             * @note This updates the internal cached DLC value.
             */
            void impl_set_dlc(std::size_t dlc);

        public:
            /**
             * @brief Get the frame version from the internal storage.
             * @return CANVersion The frame version
             */
            CANVersion impl_get_CAN_version() const;
            /**
             * @brief Set the frame version in the internal storage.
             * @param ver The CANVersion to set. Must be one of the valid enum values.
             */
            void impl_set_CAN_version(CANVersion ver);
            /**
             * @brief Get the frame format from the internal storage.
             * @return Format The frame format
             */
            Format impl_get_format() const;

            /**
             * @brief Set the frame format in the internal storage.
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
             * @param id The frame ID to set.
             */
            void impl_set_CAN_id(uint32_t id);

            /**
             * @brief Get the data length code (DLC) from the internal storage.
             * @return std::size_t The DLC value\
             * @note This uses the cached current_dlc_ variable for efficiency.
             */
            std::size_t impl_get_dlc() const;

            /**
             * @brief Get a read-only view of the data payload.
             * @return span<const std::byte> View of the data
             */
            span<const std::byte> impl_get_data() const;

            /**
             * @brief Get a modifiable view of the data payload.
             * @return span<std::byte> Mutable view of the data
             */
            span<std::byte> impl_get_data();

            /**
             * @brief Set the data payload in the internal storage.
             * @param data A span representing the data payload to set.
             */
            void impl_set_data(span<const std::byte> data);

    };
}
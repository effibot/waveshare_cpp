#pragma once

#include "base_frame.hpp"
#include "checksum_utils.hpp"

namespace USBCANBridge {
    /**
     * @brief Configuration Frame implementation.
     * This class represents a configuration frame in the USBCANBridge library.
     * It inherits from BaseFrame and provides specific functionality to apply settings to the USB-CAN adapter such as baud rate, CAN mode, ID and Mask filtering.
     * @see ConfigCommandIndex in common.hpp for details on configuration frame structure.
     * @see BaseFrame for common frame functionalities.
     */

    class ConfigFrame : public BaseFrame<ConfigFrame> {
        protected:
            using FilterType = Traits::FilterType;
            using MaskType = Traits::MaskType;
            alignas(4) mutable StorageType storage_ = {std::byte{0}};
            // dirty bit for lazy evaluation of checksum
            mutable bool dirty_ = true;
            // cached checksum value
            mutable uint8_t checksum_ = 0;

        private:
            /**
             * @brief Initialize fixed fields of the configuration frame.
             * @see ConfigCommandIndex in common.hpp for field details.
             */
            void init_fixed_fields();

        public:
            // <<< Constructors
            /**
             * @brief Default constructor.
             *
             * Initializes a ConfigFrame object with default values.
             */
            ConfigFrame() : BaseFrame() {
                init_fixed_fields();
            }
            ~ConfigFrame() = default;
            // >>> Constructors

            // ? Base Frame Protocol Interface

            Result<std::size_t> impl_size() const;
            Result<Status> impl_clear();
            Result<Type> impl_get_type() const;
            Result<Status> impl_set_type(const Type& type);
            Result<FrameType> impl_get_frame_type() const;
            Result<Status> impl_set_frame_type(const FrameType& frame_type);

            // * Wire protocol serialization/deserialization
            Result<StorageType> impl_serialize() const;
            Result<Status> impl_deserialize(const StorageType& data);

            // ? Configuration Frame specific Interface

            Result<CANBaud> get_baud_rate() const;
            Result<Status> set_baud_rate(const CANBaud& baud_rate);
            Result<CANMode> get_can_mode() const;
            Result<Status> set_can_mode(const CANMode& mode);
            Result<FilterType> get_id_filter() const;
            Result<Status> set_id_filter(const FilterType& filter);
            Result<MaskType> get_id_mask() const;
            Result<Status> set_id_mask(const MaskType& mask);

            // * Validation methods
            Result<Status> impl_validate() const;
            // ? Individual field validation methods
            Result<Status> validate_start_byte() const;
            Result<Status> validate_header_byte() const;
            Result<Status> validate_type(const Type& type) const;
            Result<Status> validate_frame_type(const FrameType& frame_type) const;
            Result<Status> validate_baud_rate(const CANBaud& baud_rate) const;
            Result<Status> validate_can_mode(const CANMode& mode) const;
            Result<Status> validate_id_filter(const FilterType& filter) const;
            Result<Status> validate_id_mask(const MaskType& mask) const;
            Result<Status> validate_reserved_byte() const;
            // ? Checksum specific methods
            /**
             * @brief Calculate checksum for the current configuration frame over the bytes:
             * header, type, baud rate, frame type, filter ID, mask ID, CAN mode, auto RTX, backup bytes.
             *
             * @return Result<uint8_t> The calculated checksum.
             */
            Result<uint8_t> calculateChecksum() const {
                return Result<uint8_t>::success(
                    ChksmUtil::calculateChecksum<ConfigFrame>(storage_)
                );
            }

            /**
             * @brief Update and cache the checksum for the current configuration frame.
             * If the frame has been modified (dirty_), recalculates the checksum.
             * @return Result<uint8_t> The updated checksum.
             */
            Result<Status> updateChecksum() const {
                // Check if cached checksum is valid
                if (dirty_) {
                    // Use polymorphic checksum utility
                    checksum_ = ChksmUtil::calculateChecksum<ConfigFrame>(storage_);
                    dirty_ = false;
                    storage_[to_size_t(ConfigCommandIndex::CHECKSUM)] =
                        static_cast<std::byte>(checksum_);
                }
                return Result<Status>::success(Status::SUCCESS);
            }
            /**
             * @brief Validate the checksum of the current configuration frame.
             * If the dirty_ flag is set, the checksum is updated before validation.
             *
             * @param frame The configuration frame to validate.
             * @return Result<Status>
             */
            Result<Status> validateChecksum(const ConfigFrame& frame) const {
                if (dirty_) {
                    // Update checksum if frame is dirty
                    auto res = updateChecksum();
                    if (res.fail()) {
                        return res;
                    }
                }
                // Use polymorphic checksum utility
                return ChksmUtil::validateChecksum<ConfigFrame>(frame.storage_);
            }

            // Static methods for raw byte stream processing
            /**
             * @brief Calculate checksum from raw byte stream (static version).
             * @param data Pointer to the raw byte data
             * @param size Size of the data buffer
             * @return uint8_t The calculated checksum
             */
            static uint8_t calculateChecksum(const std::byte* data, std::size_t size) {
                return ChksmUtil::calculateChecksum(data, size,
                    to_size_t(ConfigCommandIndex::TYPE),
                    to_size_t(ConfigCommandIndex::BACKUP_3));
            }

            /**
             * @brief Validate checksum from raw byte stream (static version).
             * @param data Pointer to the raw byte data
             * @param size Size of the data buffer
             * @return Result<Status> The result of the validation
             */
            static Result<Status> validateChecksum(const std::byte* data, std::size_t size) {
                return ChksmUtil::validateChecksum(data, size,
                    to_size_t(ConfigCommandIndex::TYPE),
                    to_size_t(ConfigCommandIndex::BACKUP_3),
                    to_size_t(ConfigCommandIndex::CHECKSUM));
            }

    };
};     // namespace USBCANBridge
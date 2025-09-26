#pragma once

#include "base_frame.hpp"
#include "checksum_utils.hpp"


namespace USBCANBridge {
    /**
     * @brief Fixed frame implementation.
     *
     * This class represents a fixed frame in the USBCANBridge library.
     * It inherits from the BaseFrame class and provides specific functionality
     * for fixed frames, such as checksum calculation and validation.
     * @see FixedSizeIndex in common.hpp for details on fixed frame structure.
     * @see BaseFrame for common frame functionalities.
     */
    class FixedFrame : public BaseFrame<FixedFrame> {
        protected:
            using PayloadPair = Traits::PayloadPair;
            using IDPair = Traits::IDPair;
            alignas(4) mutable StorageType storage_ = {std::byte{0}};
            // dirty bit for lazy evaluation of checksum
            mutable bool dirty_ = true;
            // cached checksum value
            mutable uint8_t checksum_ = 0;
            // cached id size value
            mutable size_t id_size_ = 0;
        private:
            /**
             * @brief Initialize fixed fields of the fixed frame.
             * @see FixedSizeIndex in common.hpp for field details.
             */
            void init_fixed_fields();

        public:
            // <<< Constructors
            /**
             * @brief Default constructor.
             *
             * Initializes a FixedFrame object with default values.
             */
            FixedFrame() : BaseFrame() {
                init_fixed_fields();
            }
            ~FixedFrame() = default;
            // >>> Constructors

            // ? Base Frame Protocol Interface

            Result<std::size_t> impl_size() const;
            Result<Status> impl_clear();
            Result<Type> impl_get_type() const;
            Result<Status> impl_set_type(const Type& type);
            Result<FrameType> impl_get_frame_type() const;
            Result<Status> impl_set_frame_type(const FrameType& frame_type);
            Result<FrameFmt> impl_get_frame_fmt() const;
            Result<Status> impl_set_frame_fmt(const FrameFmt& frame_fmt);

            // * Wire protocol serialization/deserialization
            Result<StorageType> impl_serialize() const;
            Result<Status> impl_deserialize(const StorageType& data);

            // ? Fixed Frame specific Interface
            /**
             * @brief Inline method to check if the frame uses an extended CAN ID,
             * based on the FrameType field. If the cached id_size_ is zero, it retrieves
             * the FrameType and updates id_size_ accordingly (4 for extended, 2 for
             * standard).
             *
             * @return Result<bool>
             */
            Result<bool> is_extended() const {
                if (id_size_ == 0) {
                    auto frame_type_res = impl_get_frame_type();
                    if (frame_type_res.fail()) {
                        return Result<bool>::error(Status::WBAD_FRAME_TYPE);
                    }
                    auto frame_type = frame_type_res.value;
                    id_size_ = (frame_type == FrameType::EXT_FIXED) ? 4 : 2;
                }
                return Result<bool>::success(id_size_ == 4);
            }
            Result<IDPair> impl_get_id() const;
            Result<Status> impl_set_id(const IDPair& id);
            Result<std::byte> impl_get_dlc() const;
            Result<Status> impl_set_dlc(const std::byte& dlc);
            Result<PayloadPair> impl_get_data() const;
            Result<Status> impl_set_data(const PayloadPair& new_data);

            // * Validation methods
            Result<Status> impl_validate() const;

            // ? Individual field validation methods
            Result<Status> validate_start_byte() const;
            Result<Status> validate_header_byte() const;
            Result<Status> validate_type(const Type& type) const;
            Result<Status> validate_frame_type(const FrameType& frame_type) const;
            Result<Status> validate_frame_fmt(const FrameFmt& frame_fmt) const;
            Result<Status> validate_id_size(const IDPair& id) const;
            Result<Status> validate_dlc(const std::byte& dlc) const;
            Result<Status> validate_dlc(const std::size_t& dlc) const;
            Result<Status> validate_reserved_byte() const;

            // ? Checksum specific methods

            // Static methods for raw byte stream processing
            /**
             * @brief Calculate checksum from raw byte stream (static version).
             *
             * This static method calculates the checksum for a fixed frame from raw bytes
             * without requiring a FixedFrame object allocation. It operates on the checksum
             * range: TYPE to RESERVED inclusive (bytes 2-18 of a 20-byte frame).
             *
             * @param data Pointer to the raw byte data (must be at least 20 bytes)
             * @param size Size of the data buffer (must be at least 20 bytes)
             * @return uint8_t The calculated checksum (low 8 bits of sum)
             */
            static uint8_t calculateChecksum(const std::byte* data, std::size_t size) {
                return ChksmUtil::calculateChecksum(data, size,
                    to_size_t(FixedSizeIndex::TYPE),
                    to_size_t(FixedSizeIndex::RESERVED));
            }

            /**
             * @brief Validate checksum from raw byte stream (static version).
             *
             * This static method validates the checksum of a fixed frame from raw bytes
             * without requiring a FixedFrame object allocation. It calculates the expected
             * checksum and compares it with the stored checksum at position 19.
             *
             * @param data Pointer to the raw byte data (must be at least 20 bytes)
             * @param size Size of the data buffer (must be at least 20 bytes)
             * @return bool True if checksum is valid, false otherwise
             */
            static Result<Status> validateChecksum(const std::byte* data, std::size_t size) {
                return ChksmUtil::validateChecksum(data, size,
                    to_size_t(FixedSizeIndex::TYPE),
                    to_size_t(FixedSizeIndex::RESERVED),
                    to_size_t(FixedSizeIndex::CHECKSUM));
            }

            /**
             * @brief Calculate the checksum for a fixed frame over the bytes:
             * type, frame_type, frame_fmt, id[4], dlc, data[8], reserved.
             * The checksum is the low 8 bits of the sum of these bytes
             * @return uint8_t The calculated checksum.
             */
            Result<uint8_t> calculateChecksum() const {
                return Result<uint8_t>::success(
                    ChksmUtil::calculateChecksum<FixedFrame>(storage_)
                );
            }
            /**
             * @brief Update and cache the checksum for the current fixed frame.
             * If the frame has been modified (dirty_), recalculates the checksum.
             * @return Result<uint8_t> The updated checksum.
             */
            Result<Status> updateChecksum() const {
                // Check if cached checksum is valid
                if (dirty_) {
                    // Use polymorphic checksum utility
                    checksum_ = ChksmUtil::calculateChecksum<FixedFrame>(storage_);
                    dirty_ = false;
                    storage_[to_size_t(FixedSizeIndex::CHECKSUM)] =
                        static_cast<std::byte>(checksum_);
                }
                return Result<Status>::success(Status::SUCCESS);
            }
            /**
             * @brief Validate the checksum of the given fixed frame.
             * This validation is intended to be used during deserialization to ensure data integrity. When validating the current object, use impl_validate() instead which also checks other fields.
             * If the dirty_ flag is set, the checksum is updated before validation.
             * @param frame The fixed frame to validate.
             * @return Result<Status> The result of the validation.
             */
            Result<Status> validateChecksum(const FixedFrame& frame) const {
                if (dirty_) {
                    // Update checksum if frame is dirty
                    auto res = updateChecksum();
                    if (res.fail()) {
                        return res;
                    }
                }
                // Use polymorphic checksum utility
                return ChksmUtil::validateChecksum<FixedFrame>(frame.storage_);
            }
    };
}
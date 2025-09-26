#include "../include/fixed_frame.hpp"

namespace USBCANBridge {
    /**
     * @brief Initialization of fixed fields in the storage array.
     * Sets constant fields to their default values.
     * This function is called during construction to ensure the frame is
     * properly initialized.
     */
    void FixedFrame::init_fixed_fields() {
        // * [START]
        storage_[to_size_t(FixedSizeIndex::START)] = to_byte(Constants::START_BYTE);
        // * [HEADER]
        storage_[to_size_t(FixedSizeIndex::HEADER)] = to_byte(Constants::MSG_HEADER);
        // * [TYPE]
        storage_[to_size_t(FixedSizeIndex::TYPE)] = to_byte(Type::DATA_FIXED);
        // * [FRAME_TYPE]
        storage_[to_size_t(FixedSizeIndex::FRAME_TYPE)] = to_byte(FrameType::STD_FIXED);
        id_size_ = 2; // default to standard ID size
        // * [FRAME_FMT]
        storage_[to_size_t(FixedSizeIndex::FRAME_FMT)] = to_byte(FrameFmt::DATA_FIXED);
        // * [RESERVED]
        storage_[to_size_t(FixedSizeIndex::RESERVED)] = to_byte(Constants::RESERVED0);
        // ? [CHECKSUM] - will be calculated on first serialize() call
        dirty_ = true; // mark checksum as dirty
        checksum_ = 0; // initialize checksum to zero
    }

    // ? Base Frame Protocol Interface
    /**
     * @brief Get the size of the fixed frame.
     *
     * @return Result<std::size_t>
     */
    Result<std::size_t> FixedFrame::impl_size() const {
        return Result<std::size_t>::success(Traits::FRAME_SIZE);
    }
    /**
     * @brief Clear the frame to default state.
     * Resets all fields to zero and reinitializes fixed fields.
     * Marks the checksum as dirty for recalculation.
     *
     * @return Result<Status>
     */
    Result<Status> FixedFrame::impl_clear() {
        // Reset to default state
        std::fill(storage_.begin(), storage_.end(), std::byte{0});
        // Reinitialize fixed fields
        init_fixed_fields();
        // Mark checksum as dirty
        dirty_ = true;
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Get the Type field of the frame.
     *
     * For FixedFrame, this should always be Type::DATA_FIXED.
     *
     * @return Result<Type>
     */
    Result<Type> FixedFrame::impl_get_type() const {
        return Result<Type>::success(
            from_byte<Type>(storage_[to_size_t(FixedSizeIndex::TYPE)])
        );
    }
    /**
     * @brief Set the Type field of the frame.
     *
     * For FixedFrame, this should always be Type::DATA_FIXED.
     *
     * @param type
     * @return Result<Status>
     */
    Result<Status> FixedFrame::impl_set_type(const Type& type) {
        // Validate type for fixed frame
        auto validation_result = validate_type(type);
        if (validation_result.fail()) {
            return validation_result;
        }
        storage_[to_size_t(FixedSizeIndex::TYPE)] = to_byte(type);
        // Mark checksum as dirty
        dirty_ = true;
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Get the FrameType field of the frame.
     *
     * For FixedFrame, this can be FrameType::STD_FIXED or FrameType::EXT_FIXED.
     *
     * @return Result<FrameType>
     */
    Result<FrameType> FixedFrame::impl_get_frame_type() const {
        return Result<FrameType>::success(
            from_byte<FrameType>(storage_[to_size_t(FixedSizeIndex::FRAME_TYPE)])
        );
    }
    /**
     * @brief Set the FrameType field of the frame.
     * For FixedFrame, only FrameType::STD_FIXED and FrameType::EXT_FIXED are valid.
     * When setting the FrameType, the cached id_size_ is re-evaluated by calling the is_extended() method automatically.
     * @param frame_type New FrameType value.
     * @return Result<Status>
     */
    Result<Status> FixedFrame::impl_set_frame_type(const FrameType& frame_type) {
        // Validate frame type for fixed frame
        auto validation_result = validate_frame_type(frame_type);
        if (validation_result.fail()) {
            return validation_result;
        }
        storage_[to_size_t(FixedSizeIndex::FRAME_TYPE)] = to_byte(frame_type);
        // Mark checksum as dirty
        dirty_ = true;
        // Cache the ID size based on type
        id_size_ = 0; // reset cached id_size_ to force re-evaluation
        is_extended(); // update id_size_ based on current FrameType
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Get the FrameFmt field of the frame.
     *
     * For FixedFrame, this can be FrameFmt::DATA_FIXED or FrameFmt::REMOTE_FIXED.
     *
     * @return Result<FrameFmt>
     */
    Result<FrameFmt> FixedFrame::impl_get_frame_fmt() const {
        return Result<FrameFmt>::success(
            from_byte<FrameFmt>(storage_[to_size_t(FixedSizeIndex::FRAME_FMT)])
        );
    }
    /**
     * @brief Set the FrameFmt field of the frame.
     *
     * For FixedFrame, this should be either FrameFmt::DATA_FIXED or FrameFmt::REMOTE_FIXED.
     *
     * @param frame_fmt
     * @return Result<Status>
     */
    Result<Status> FixedFrame::impl_set_frame_fmt(const FrameFmt& frame_fmt) {
        // Validate frame format for fixed frame
        auto validation_result = validate_frame_fmt(frame_fmt);
        if (validation_result.fail()) {
            return validation_result;
        }
        storage_[to_size_t(FixedSizeIndex::FRAME_FMT)] = to_byte(frame_fmt);
        // Mark checksum as dirty
        dirty_ = true;
        return Result<Status>::success(Status::SUCCESS);
    }

    // * Wire protocol serialization/deserialization
    /**
     * @brief Serialize the frame into a byte array.
     * Validates the frame and return a copy of the internal storage with updated checksum.
     *
     * @return Result<StorageType>
     */
    Result<FixedFrame::StorageType> FixedFrame::impl_serialize() const {
        // Update checksum if dirty
        updateChecksum();

        // validate the entire frame before serialization
        auto validation_res = impl_validate();
        if (validation_res.fail()) {
            // initialize an array with error status for interface consistency
            StorageType error_storage = {to_byte(Status::WBAD_DATA)};
            Result<StorageType> res = Result<StorageType>();
            res.value = error_storage;
            res.status = validation_res.status;
            return res;
        }

        // If everything is valid, proceed with serialization
        StorageType storage_copy = storage_;

        return Result<StorageType>::success(storage_copy);
    }
    /**
     * @brief Deserialize a byte array into the config frame.
     * The caller must prepare a valid StorageType array of correct size.
     * @see frame_traits.hpp for details on StorageType<T>
     *
     * @param data
     * @return Result<Status>
     */
    Result<Status> FixedFrame::impl_deserialize(const StorageType& data) {
        // Validate input size
        if (data.size() != Traits::FRAME_SIZE) {
            return Result<Status>::error(Status::WBAD_LENGTH);
        }
        // Copy data into internal storage
        storage_ = data;
        dirty_   = true; // mark checksum as dirty for recalculation
        // Validate the entire frame after deserialization
        auto res = impl_validate();
        if (res.fail()) {
            return res;
        }
        return Result<Status>::success(Status::SUCCESS);
    }

    // ? Fixed Frame specific Interface
    /**
     * @brief Get the ID field of the frame as a pair of byte array
     * and the size allowed by the current FrameType value.
     *
     * @return Result<FixedFrame::IDPair>
     */
    Result<FixedFrame::IDPair> FixedFrame::impl_get_id() const {
        IDPair id;
        // ! find how many bytes to copy based on FrameType
        size_t n_bytes = is_extended() ? 4 : 2;
        // Copy ID bytes from storage
        std::copy_n(
            storage_.begin() + to_size_t(FixedSizeIndex::ID_0),
            n_bytes,
            id.first.begin()
        );
        id.second = n_bytes;
        return Result<FixedFrame::IDPair>::success(id);
    }
    /**
     * @brief Set the ID field of the frame.
     * When invoked, this method validates the size of the provided ID
     * against the current FrameType (2 bytes for standard, 4 bytes for extended).
     *
     * @param id
     * @return Result<Status>
     */
    Result<Status> FixedFrame::impl_set_id(const IDPair& id) {
        // Validate ID size against current FrameType
        auto validation_result = validate_id_size(id);
        if (validation_result.fail()) {
            return validation_result;
        }
        // Copy ID bytes into storage
        std::copy_n(
            id.first.begin(),
            id.second,
            storage_.begin() + to_size_t(FixedSizeIndex::ID_0)
        );
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Get the Data Length Code (DLC) field of the frame.
     *
     * @return Result<std::byte>
     */
    Result<std::byte> FixedFrame::impl_get_dlc() const {
        return Result<std::byte>::success(
            storage_[to_size_t(FixedSizeIndex::DLC)]
        );
    }
    /**
     * @brief Set the Data Length Code (DLC) field of the frame.
     * Validates that the DLC is in the range 0-8.
     * Marks the checksum as dirty for recalculation.
     * This will directly affect how many data bytes are considered valid.
     * @param dlc
     * @return Result<Status>
     */
    Result<Status> FixedFrame::impl_set_dlc(const std::byte& dlc) {
        // Validate DLC range (0-8)
        auto validation_result = validate_dlc(dlc);
        if (validation_result.fail()) {
            return validation_result;
        }
        storage_[to_size_t(FixedSizeIndex::DLC)] = dlc;
        // Mark checksum as dirty
        dirty_ = true;
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Get a copy of the entire data payload and DLC from the frame.
     *
     * @return Result<FixedFrame::PayloadPair>
     */
    Result<FixedFrame::PayloadPair> FixedFrame::impl_get_data() const {
        PayloadPair data;
        // Copy DLC
        data.second = to_size_t(storage_[to_size_t(FixedSizeIndex::DLC)]);
        // Copy data payload
        std::copy_n(
            storage_.begin() + to_size_t(FixedSizeIndex::DATA_0),
            data.second,
            data.first.begin()
        );
        return Result<FixedFrame::PayloadPair>::success(data);
    }
    /**
     * @brief Set a new data payload and DLC for the frame.
     *
     * @param new_data
     * @return Result<Status>
     */
    Result<Status> FixedFrame::impl_set_data(const PayloadPair& new_data) {
        // Validate DLC range
        auto validation_result = validate_dlc(new_data.second);
        if (validation_result.fail()) {
            return validation_result;
        }
        // Copy new bytes into the frame data field
        auto frame_start = storage_.begin() + to_size_t(FixedSizeIndex::DATA_0);
        std::copy(new_data.first.begin(), new_data.first.begin() + new_data.second, frame_start);
        // Zero-fill unused bytes
        std::fill(frame_start + new_data.second, frame_start + 8, std::byte{0});
        // Update DLC
        storage_[to_size_t(FixedSizeIndex::DLC)] = static_cast<std::byte>(new_data.second);
        // Mark dirty for checksum recalculation
        dirty_ = true;
        return Result<Status>::success(Status::SUCCESS);
    }

    // * Validation methods
    /**
     * @brief Validate the integrity of the frame.
     * A frame is considered valid if:
     * - Start byte is 0xAA
     * - Header byte is 0x55
     * - Type is DATA_FIXED
     * - FrameType is STD_FIXED or EXT_FIXED
     * - FrameFmt is DATA_FIXED or REMOTE_FIXED
     * - DLC is in range 0-8
     * - Reserved byte is 0x00
     * - Checksum matches the computed checksum
     *
     * @return Result<Status>
     */
    Result<Status> FixedFrame::impl_validate() const {
        // Validate start byte
        auto start_result = validate_start_byte();
        if (start_result.fail()) {
            return start_result;
        }

        // Validate header byte
        auto header_result = validate_header_byte();
        if (header_result.fail()) {
            return header_result;
        }

        // Validate type
        Type current_type = impl_get_type().value;
        auto type_result = validate_type(current_type);
        if (type_result.fail()) {
            return type_result;
        }

        // Validate frame type
        FrameType frame_type = impl_get_frame_type().value;
        auto frame_type_result = validate_frame_type(frame_type);
        if (frame_type_result.fail()) {
            return frame_type_result;
        }

        // Validate frame format
        FrameFmt frame_fmt = impl_get_frame_fmt().value;
        auto frame_fmt_result = validate_frame_fmt(frame_fmt);
        if (frame_fmt_result.fail()) {
            return frame_fmt_result;
        }

        // Validate DLC
        std::byte dlc_byte = impl_get_dlc().value;
        auto dlc_result = validate_dlc(dlc_byte);
        if (dlc_result.fail()) {
            return dlc_result;
        }

        // Validate reserved byte
        auto reserved_result = validate_reserved_byte();
        if (reserved_result.fail()) {
            return reserved_result;
        }

        // Validate checksum
        auto checksum_result = validateChecksum(*this);
        if (checksum_result.fail()) {
            return checksum_result;
        }
        return Result<Status>::success(Status::SUCCESS);
    }

    // ? Individual field validation methods
    /**
     * @brief Validate the start byte of the frame.
     *
     * @return Result<Status> SUCCESS if start byte is 0xAA, WBAD_START otherwise
     */
    Result<Status> FixedFrame::validate_start_byte() const {
        if (storage_[to_size_t(FixedSizeIndex::START)] != to_byte(Constants::START_BYTE)) {
            return Result<Status>::error(Status::WBAD_START);
        }
        return Result<Status>::success(Status::SUCCESS);
    }

    /**
     * @brief Validate the header byte of the frame.
     *
     * @return Result<Status> SUCCESS if header byte is 0x55, WBAD_HEADER otherwise
     */
    Result<Status> FixedFrame::validate_header_byte() const {
        if (storage_[to_size_t(FixedSizeIndex::HEADER)] != to_byte(Constants::MSG_HEADER)) {
            return Result<Status>::error(Status::WBAD_HEADER);
        }
        return Result<Status>::success(Status::SUCCESS);
    }

    /**
     * @brief Validate the type field of the frame.
     *
     * @param type The type to validate
     * @return Result<Status> SUCCESS if type is DATA_FIXED, WBAD_TYPE otherwise
     */
    Result<Status> FixedFrame::validate_type(const Type& type) const {
        if (type != Type::DATA_FIXED) {
            return Result<Status>::error(Status::WBAD_TYPE);
        }
        return Result<Status>::success(Status::SUCCESS);
    }

    /**
     * @brief Validate the frame type field.
     *
     * @param frame_type The frame type to validate
     * @return Result<Status> SUCCESS if frame_type is STD_FIXED or EXT_FIXED, WBAD_TYPE otherwise
     */
    Result<Status> FixedFrame::validate_frame_type(const FrameType& frame_type) const {
        if (frame_type != FrameType::STD_FIXED && frame_type != FrameType::EXT_FIXED) {
            return Result<Status>::error(Status::WBAD_TYPE);
        }
        return Result<Status>::success(Status::SUCCESS);
    }

    /**
     * @brief Validate the frame format field.
     *
     * @param frame_fmt The frame format to validate
     * @return Result<Status> SUCCESS if frame_fmt is DATA_FIXED or REMOTE_FIXED, WBAD_FORMAT otherwise
     */
    Result<Status> FixedFrame::validate_frame_fmt(const FrameFmt& frame_fmt) const {
        if (frame_fmt != FrameFmt::DATA_FIXED && frame_fmt != FrameFmt::REMOTE_FIXED) {
            return Result<Status>::error(Status::WBAD_FORMAT);
        }
        return Result<Status>::success(Status::SUCCESS);
    }

    /**
     * @brief Validate the ID size against the current frame type.
     *
     * @param id The ID pair to validate (contains both ID bytes and size)
     * @return Result<Status> SUCCESS if ID size matches frame type requirements, WBAD_ID otherwise
     */
    Result<Status> FixedFrame::validate_id_size(const IDPair& id) const {
        size_t expected_size = is_extended().value ? 4 : 2;
        if (id.second != expected_size) {
            return Result<Status>::error(Status::WBAD_ID);
        }
        return Result<Status>::success(Status::SUCCESS);
    }

    /**
     * @brief Validate the Data Length Code (byte version).
     *
     * @param dlc The DLC to validate as std::byte
     * @return Result<Status> SUCCESS if DLC is in range 0-8, WBAD_DLC otherwise
     */
    Result<Status> FixedFrame::validate_dlc(const std::byte& dlc) const {
        auto dlc_value = static_cast<uint>(dlc);
        if (dlc_value > 8) {
            return Result<Status>::error(Status::WBAD_DLC);
        }
        return Result<Status>::success(Status::SUCCESS);
    }

    /**
     * @brief Validate the Data Length Code (size_t version).
     *
     * @param dlc The DLC to validate as std::size_t
     * @return Result<Status> SUCCESS if DLC is in range 0-8, WBAD_DLC otherwise
     */
    Result<Status> FixedFrame::validate_dlc(const std::size_t& dlc) const {
        if (dlc > 8) {
            return Result<Status>::error(Status::WBAD_DLC);
        }
        return Result<Status>::success(Status::SUCCESS);
    }

    /**
     * @brief Validate the reserved byte of the frame.
     *
     * @return Result<Status> SUCCESS if reserved byte is 0x00, WBAD_RESERVED otherwise
     */
    Result<Status> FixedFrame::validate_reserved_byte() const {
        if (storage_[to_size_t(FixedSizeIndex::RESERVED)] != to_byte(Constants::RESERVED0)) {
            return Result<Status>::error(Status::WBAD_RESERVED);
        }
        return Result<Status>::success(Status::SUCCESS);
    }

}
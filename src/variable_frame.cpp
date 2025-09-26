#include "../include/variable_frame.hpp"

namespace USBCANBridge {
    /**
     * @brief Initialize fixed fields of the variable frame.
     * For VariableFrame, we only know the position of the start byte and type byte.
     * The rest of the fields will be zeroed out.
     * @note The Type byte is initialized to `DATA_VAR` (`{0xC0}`) by default, which sets the first two bits to `1`.
     */
    void VariableFrame::init_fixed_fields() {
        // Initialize storage with minimum size
        storage_.resize(Traits::MIN_FRAME_SIZE);
        std::fill(storage_.begin(), storage_.end(), std::byte{0x00});

        // * [START]
        storage_[0] = to_byte(Constants::START_BYTE);
        // * [TYPE{0:2}] (init the first two bits to `11` for variable frame)
        storage_[1] = to_byte(Type::DATA_VAR);
        // * [END] - at the last position
        storage_[storage_.size() - 1] = to_byte(Constants::END_BYTE);
    }
    /**
     * @brief Get the size of the variable frame.
     *
     * The size is dynamic based on the DLC field and ID size.
     * Minimum size is 5 bytes (no ID, no data).
     * Maximum size is 15 bytes (4-byte ID, 8-byte data).
     *
     * @return Result<std::size_t> Size of the frame in bytes
     */
    Result<std::size_t> VariableFrame::impl_size() const {
        return Result<std::size_t>::success(storage_.size());
    }
    /**
     * @brief Clear the frame to default state.
     * Resets all fields to zero and reinitializes fixed fields.
     *
     * @return Result<Status>
     */
    Result<Status> VariableFrame::impl_clear() {
        // Reset to default state
        storage_.resize(Traits::MIN_FRAME_SIZE);
        init_fixed_fields();
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Get the Type field of the frame.
     *
     * For VariableFrame, this should always be Type::DATA_VAR.
     *
     * @return Result<Type>
     */
    Result<Type> VariableFrame::impl_get_type() const {
        return Result<Type>::success(
            from_byte<Type>(storage_[1])
        );
    }
    /**
     * @brief Set the Type field of the frame.
     *
     * For VariableFrame, the Type is computed based on FrameFmt, FrameType and DLC.
     * This method is disabled to prevent direct setting of Type.
     *
     * @param type
     * @return Result<Status>
     */
    // Result<Status> VariableFrame::impl_set_type(const Type& type) {
    //     // Direct setting of Type is not allowed for VariableFrame
    //     return Result<Status>::error(Status::WBAD_TYPE);
    // }
    /**
     * @brief Set the Type field based on FrameFmt, FrameType and DLC.
     * Computes the appropriate Type value and updates the frame.
     * @param frame_fmt Frame format (standard or extended)
     * @param frame_type Frame type (fixed or variable)
     * @param dlc Data Length Code (0-8)
     * @return Result<Status>
     */
    Result<Status> VariableFrame::impl_set_type(const FrameFmt frame_fmt,
        const FrameType frame_type, const std::byte dlc) {
        // Validate inputs
        auto fmt_res = validate_frame_fmt(frame_fmt);
        if (fmt_res.fail()) {
            return fmt_res; // Propagate error
        }
        auto type_res = validate_frame_type(frame_type);
        if (type_res.fail()) {
            return type_res; // Propagate error
        }
        auto dlc_res = validate_dlc(dlc);
        if (dlc_res.fail()) {
            return dlc_res; // Propagate error
        }
        // Compute the Type value based on validated inputs
        std::byte type_value = compute_type(frame_type, frame_fmt, dlc);
        storage_[1] = type_value;
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Get the FrameType field of the frame.
     * The FrameType is determined by bit 5 of the Type byte.
     * @return Result<FrameType>
     */
    Result<FrameType> VariableFrame::impl_get_frame_type() const {
        FrameType frame_type =
            (storage_[1]& std::byte{0x20}) !=
            std::byte{0} ? FrameType::EXT_VAR : FrameType::STD_VAR;
        return Result<FrameType>::success(frame_type);
    }
    /**
     * @brief Set the FrameType field of the frame.
     * Validates the input and updates the Frame Type bit accordingly.
     * @param frame_type Frame type (fixed or variable)
     * @return Result<Status>
     */
    Result<Status> VariableFrame::impl_set_frame_type(const FrameType& frame_type) {
        auto res = validate_frame_type(frame_type);
        if (res.fail()) {
            return res; // Propagate error
        }

        // Set the FrameType field in the storage (bit 5)
        std::uint8_t current = static_cast<std::uint8_t>(storage_[1]);
        current = (current & 0xDF) | ((to_size_t(frame_type) & 0x01) << 5);
        storage_[1] = static_cast<std::byte>(current);
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Get the FrameFmt field of the frame.
     * The FrameFmt is determined by bit 4 of the Type byte.
     * @return Result<FrameFmt>
     */
    Result<FrameFmt> VariableFrame::impl_get_frame_fmt() const {
        FrameFmt frame_fmt =
            (storage_[1]& std::byte{0x10}) !=
            std::byte{0} ? FrameFmt::REMOTE_VAR : FrameFmt::DATA_VAR;
        return Result<FrameFmt>::success(frame_fmt);
    }
    /**
     * @brief Set the FrameFmt field of the frame.
     * Validates the input and updates the Frame Format bit accordingly.
     * @param frame_fmt Frame format (standard or extended)
     * @return Result<Status>
     */
    Result<Status> VariableFrame::impl_set_frame_fmt(const FrameFmt& frame_fmt) {
        auto res = validate_frame_fmt(frame_fmt);
        if (res.fail()) {
            return res; // Propagate error
        }
        // Set the FrameFmt field in the storage (bit 4)
        std::uint8_t current = static_cast<std::uint8_t>(storage_[1]);
        current = (current & 0xEF) | ((to_size_t(frame_fmt) & 0x01) << 4);
        storage_[1] = static_cast<std::byte>(current);
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Serialize the frame to a byte array.
     *
     * @return Result<StorageType> Serialized byte array
     */
    Result<VariableFrame::StorageType> VariableFrame::impl_serialize() const {
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
     * @brief Deserialize a byte array into the frame.
     * Validates the input data and updates internal storage.
     *
     * @param data Input byte array
     * @param size Size of the input data
     * @return Result<Status>
     */
    Result<Status> VariableFrame::impl_deserialize(const std::byte* data, std::size_t size) {
        // Validate input size
        if (size < Traits::MIN_FRAME_SIZE || size > Traits::MAX_FRAME_SIZE) {
            return Result<Status>::error(Status::WBAD_LENGTH);
        }

        // Validate and copy the input data
        storage_.resize(size); // Adjust size to actual data length
        std::copy_n(data, size, storage_.begin());
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Get the ID field of the frame as a pair of byte array
     * and the size allowed by the current FrameType value.
     *
     * @return Result<VariableFrame::IDPair>
     */
    Result<VariableFrame::IDPair> VariableFrame::impl_get_id() const {
        IDPair id;
        // Determine ID size based on FrameType
        auto frame_type_res = impl_get_frame_type();
        if (frame_type_res.fail()) {
            return Result<IDPair>::error(Status::WBAD_FRAME_TYPE);
        }
        auto frame_type = frame_type_res.value;
        size_t id_size = (frame_type == FrameType::EXT_VAR) ? 4 : 2;
        id.second = id_size;
        // Copy ID bytes from storage
        std::visit([&](auto& id_array) {
                std::copy_n(
                storage_.begin() + 2, // ID starts after START and TYPE bytes
                id_size,
                id_array.begin()
                );
            }, id.first);
        return Result<VariableFrame::IDPair>::success(id);
    }
    /**
     * @brief Set the ID field of the frame.
     * Validates the ID size against the current FrameType.
     *
     * @param id The ID pair to set (contains both ID bytes and size)
     * @return Result<Status>
     */
    Result<Status> VariableFrame::impl_set_id(const IDPair& id) {
        // Validate ID size against current FrameType
        auto validation_result = validate_id_size(id);
        if (validation_result.fail()) {
            return validation_result;
        }
        // Copy ID bytes into storage using std::visit to access the variant
        std::visit([&](const auto& id_array) {
                std::copy_n(
                id_array.begin(),
                id.second,
                storage_.begin() + 2 // ID starts after START and TYPE bytes
                );
            }, id.first);
        return Result<Status>::success(Status::SUCCESS);
    }

    /**
     * @brief Get the Data Length Code (DLC) field of the frame.
     * This can be retrieve from the bits 0-3 of the TYPE byte.
     * @return Result<std::byte>
     */
    Result<std::byte> VariableFrame::impl_get_dlc() const {
        std::byte dlc = storage_[1]& std::byte{0x0F};
        return Result<std::byte>::success(dlc);
    }
    /**
     * @brief Set the Data Length Code (DLC) field of the frame.
     * Validates that the DLC is in the range 0-8.
     * This will directly affect how many data bytes are considered valid.
     * @param dlc
     * @return Result<Status>
     */
    Result<Status> VariableFrame::impl_set_dlc(const std::byte& dlc) {
        // Validate DLC range (0-8)
        auto validation_result = validate_dlc(dlc);
        if (validation_result.fail()) {
            return validation_result;
        }
        // Clear existing DLC bits and set new DLC
        storage_[1] = (storage_[1]& std::byte{0xF0}) | (dlc& std::byte{0x0F});
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Get a copy of the entire data payload and DLC from the frame.
     * @return Result<VariableFrame::PayloadPair>
     */
    Result<VariableFrame::PayloadPair> VariableFrame::impl_get_data() const {
        PayloadPair data;
        // Get DLC
        auto dlc_res = impl_get_dlc();
        if (dlc_res.fail()) {
            return Result<PayloadPair>::error(Status::WBAD_DLC);
        }
        data.second = to_size_t(dlc_res.value);
        // Copy data payload based on DLC
        std::copy_n(
            storage_.begin() + 2 + id_size_, // Data starts after START, TYPE and ID bytes
            data.second,
            std::back_inserter(data.first)
        );
        return Result<PayloadPair>::success(data);
    }
    /**
     * @brief Set a new data payload and DLC for the frame.
     * Validates the DLC and adjusts the frame size accordingly.
     * @param new_data The new data payload and DLC to set
     * @return Result<Status>
     */
    Result<Status> VariableFrame::impl_set_data(const PayloadPair& new_data) {
        // Validate DLC range
        auto validation_result = validate_dlc(new_data.second);
        if (validation_result.fail()) {
            return validation_result;
        }
        // Resize storage to accommodate new data
        id_size_ = (impl_get_frame_type().value == FrameType::EXT_VAR) ? 4 : 2;
        size_t new_size = 2 + id_size_ + new_data.second + 1; // START + TYPE + ID + DATA + END
        storage_.resize(new_size);
        // Copy new bytes into the frame data field
        auto frame_start = storage_.begin() + 2 + id_size_; // Data starts after START, TYPE and ID
        std::copy(new_data.first.begin(), new_data.first.begin() + new_data.second, frame_start);
        // Zero-fill unused bytes if any (not strictly necessary in variable frame)
        std::fill(frame_start + new_data.second, storage_.end() - 1, std::byte{0});
        // Set the END byte at the last position
        storage_[storage_.size() - 1] = to_byte(Constants::END_BYTE);
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Validate the entire frame for correctness.
     * Checks all individual fields and overall structure.
     * @return Result<Status>
     */
    Result<Status> VariableFrame::impl_validate() const {
        // Validate start byte
        auto res = validate_start_byte();
        if (res.fail()) {
            return res; // Propagate error
        }
        // Validate the FrameFmt
        auto frame_fmt_res = impl_get_frame_fmt();
        res = validate_frame_fmt(frame_fmt_res.value);
        if (res.fail()) {
            return res; // Propagate error
        }
        // Validate the FrameType
        auto frame_type_res = impl_get_frame_type();
        res = validate_frame_type(frame_type_res.value);
        if (res.fail()) {
            return res; // Propagate error
        }
        // Validate ID size
        auto id_res = impl_get_id();
        res = validate_id_size(id_res.value);
        if (res.fail()) {
            return res; // Propagate error
        }
        // Validate DLC
        auto dlc_res = impl_get_dlc();
        res = validate_dlc(dlc_res.value);
        if (res.fail()) {
            return res; // Propagate error
        }
        // Validate END byte
        res = validate_end_byte();
        if (res.fail()) {
            return res; // Propagate error
        }

        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Validate the start byte of the frame.
     * @return Result<Status>
     */
    Result<Status> VariableFrame::validate_start_byte() const {
        if (storage_[0] != to_byte(Constants::START_BYTE)) {
            return Result<Status>::error(Status::WBAD_START);
        }
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Validate the Frame Format (FrameFmt) field of the frame.
     * @param frame_fmt The FrameFmt value to validate
     * @return Result<Status>
     */
    Result<Status> VariableFrame::validate_frame_fmt(const FrameFmt& frame_fmt) const {
        if (frame_fmt != FrameFmt::DATA_VAR && frame_fmt != FrameFmt::REMOTE_VAR) {
            return Result<Status>::error(Status::WBAD_FORMAT);
        }
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Validate the Frame Type (FrameType) field of the frame.
     * @param frame_type The FrameType value to validate
     * @return Result<Status>
     */
    Result<Status> VariableFrame::validate_frame_type(const FrameType& frame_type) const {
        if (frame_type != FrameType::STD_VAR && frame_type != FrameType::EXT_VAR) {
            return Result<Status>::error(Status::WBAD_FRAME_TYPE);
        }
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Validate the ID size against the current FrameType.
     * @param id The ID pair to validate (contains both ID bytes and size)
     * @return Result<Status>
     */
    Result<Status> VariableFrame::validate_id_size(const IDPair& id) const {
        size_t expected_size = (impl_get_frame_type().value == FrameType::EXT_VAR) ? 4 : 2;
        if (id.second != expected_size) {
            return Result<Status>::error(Status::WBAD_ID);
        }
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Validate the Data Length Code (DLC) field of the frame.
     * @param dlc The DLC value to validate
     * @return Result<Status>
     */
    Result<Status> VariableFrame::validate_dlc(const std::byte& dlc) const {
        return validate_dlc(to_size_t(dlc));
    }
    /**
     * @brief Validate the Data Length Code (DLC) field of the frame.
     * @param dlc The DLC value to validate
     * @return Result<Status>
     */
    Result<Status> VariableFrame::validate_dlc(const std::size_t& dlc) const {
        if (dlc > 8) {
            return Result<Status>::error(Status::WBAD_DLC);
        }
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Validate the END byte of the frame.
     * @return Result<Status>
     */
    Result<Status> VariableFrame::validate_end_byte() const {
        if (storage_.back() != to_byte(Constants::END_BYTE)) {
            return Result<Status>::error(Status::WBAD_DATA);
        }
        return Result<Status>::success(Status::SUCCESS);
    }
}
#include "../include/config_frame.hpp"
namespace USBCANBridge {
    void ConfigFrame::init_fixed_fields() {
        // * [START]
        storage_[to_size_t(ConfigCommandIndex::START)] = to_byte(Constants::START_BYTE);
        // * [HEADER]
        storage_[to_size_t(ConfigCommandIndex::HEADER)] = to_byte(Constants::MSG_HEADER);
        // * [TYPE]
        storage_[to_size_t(ConfigCommandIndex::TYPE)] = to_byte(Type::CONF_VAR);
        // * [CAN_BAUD]
        storage_[to_size_t(ConfigCommandIndex::CAN_BAUD)] = to_byte(CANBaud::SPEED_1000K); // Default 1Mbps
        // * [FRAME_TYPE]
        storage_[to_size_t(ConfigCommandIndex::FRAME_TYPE)] = to_byte(FrameType::STD_FIXED); // Default standard variable
        // * [FILTER_ID1-FILTER_ID4] [MASK_ID1-MASK_ID4] (no filtering/masking)
        std::size_t start = to_size_t(ConfigCommandIndex::FILTER_ID_1);
        std::size_t end = to_size_t(ConfigCommandIndex::MASK_ID_4);
        // Zero out filter and mask bytes
        for (std::size_t i = start; i <= end; i++) {
            storage_[i] = std::byte{0x00};
        }
        // Zero out backup bytes separately
        storage_[to_size_t(ConfigCommandIndex::BACKUP_0)] = std::byte{0x00};
        storage_[to_size_t(ConfigCommandIndex::BACKUP_1)] = std::byte{0x00};
        storage_[to_size_t(ConfigCommandIndex::BACKUP_2)] = std::byte{0x00};
        storage_[to_size_t(ConfigCommandIndex::BACKUP_3)] = std::byte{0x00};
        // * [CAN_MODE]
        storage_[to_size_t(ConfigCommandIndex::CAN_MODE)] = to_byte(CANMode::NORMAL); // Default normal mode
        // * [AUTO_RTX]
        storage_[to_size_t(ConfigCommandIndex::AUTO_RTX)] = to_byte(RTX::AUTO); // Default automatic retransmission
        // ? [CHECKSUM] - will be calculated on first serialize() call
        dirty_ = true; // mark checksum as dirty
        checksum_ = 0; // initialize checksum to zero
    }

    /**
     * @brief Get the size of the config frame.
     *
     * @return Result<std::size_t>
     */
    Result<std::size_t> ConfigFrame::impl_size() const {
        return Result<std::size_t>::success(Traits::FRAME_SIZE);
    }

    /**
     * @brief Clear the config frame.
     * Resets all fields to default values.
     * Marks the checksum as dirty for recalculation.
     * @return Result<Status>
     */
    Result<Status> ConfigFrame::impl_clear() {
        init_fixed_fields();
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Get the type of the config frame.
     * For ConfigFrame, this should always be Type::CONF_VAR or Type::CONF_FIXED.
     * @return Result<Type>
     */
    Result<Type> ConfigFrame::impl_get_type() const {
        Type type = static_cast<Type>(storage_[to_size_t(ConfigCommandIndex::TYPE)]);
        return Result<Type>::success(type);
    }
    /**
     * @brief Set the type of the config frame.
     * Validates the type before setting.
     * For ConfigFrame, valid types are Type::CONF_VAR and Type::CONF_FIXED.
     * @param type
     * @return Result<Status>
     */
    Result<Status> ConfigFrame::impl_set_type(const Type& type) {
        auto res = validate_type(type);
        if (res.fail()) {
            return res;
        }
        storage_[to_size_t(ConfigCommandIndex::TYPE)] = std::byte{static_cast<uint8_t>(type)};
        dirty_ = true;
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Get the frame type of the config frame.
     * For ConfigFrame, this can be any STD_FIXED or EXT_FIXED.
     *
     * @return Result<FrameType>
     */
    Result<FrameType> ConfigFrame::impl_get_frame_type() const {
        FrameType frame_type =
            static_cast<FrameType>(storage_[to_size_t(ConfigCommandIndex::FRAME_TYPE)]);
        return Result<FrameType>::success(frame_type);
    }

    /**
     * @brief Set the frame type of the config frame.
     * Validates the frame type before setting.
     * For ConfigFrame, valid frame types are STD_FIXED or EXT_FIXED.
     *
     * @param frame_type
     * @return Result<Status>
     */
    Result<Status> ConfigFrame::impl_set_frame_type(const FrameType& frame_type) {
        auto res = validate_frame_type(frame_type);
        if (res.fail()) {
            return res;
        }
        storage_[to_size_t(ConfigCommandIndex::FRAME_TYPE)] =
            std::byte{static_cast<uint8_t>(frame_type)};
        dirty_   = true;
        return Result<Status>::success(Status::SUCCESS);
    }
    // * Wire protocol serialization/deserialization
    /**
     * @brief Serialize the config frame to a byte array.
     * Validates the frame and return a copy of the internal storage with updated checksum.
     *
     * @return Result<StorageType>
     */
    Result<ConfigFrame::StorageType> ConfigFrame::impl_serialize() const {
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
    Result<Status> ConfigFrame::impl_deserialize(const StorageType& data) {
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
    /**
     * @brief Deserialize a byte array into the config frame.
     * Validates the input data and updates internal storage if valid.
     *
     * @param data
     * @return Result<Status>
     */
    // Result<Status> ConfigFrame::impl_deserialize(const StorageType& data){}

    // ? Configuration Frame specific Interface
    /**
     * @brief Get the CAN baud rate from the config frame.
     * @see common.hpp for CANBaud enum details and possible values.
     */
    Result<CANBaud> ConfigFrame::get_baud_rate() const {
        return Result<CANBaud>::success(from_byte<CANBaud>(
            storage_[to_size_t(ConfigCommandIndex::CAN_BAUD)]
        ));

    }
    /**
     * @brief Set the CAN baud rate in the config frame.
     * Validates the baud rate before setting.
     * @see common.hpp for CANBaud enum details and possible values.
     *
     * @param baud_rate
     * @return Result<Status>
     */
    Result<Status> ConfigFrame::set_baud_rate(const CANBaud& baud_rate) {
        auto res = validate_baud_rate(baud_rate);
        if (res.fail()) {
            return res;
        }
        storage_[to_size_t(ConfigCommandIndex::CAN_BAUD)] =
            std::byte{static_cast<uint8_t>(baud_rate)};
        dirty_   = true;
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Get the CAN mode from the config frame.
     * @see common.hpp for CANMode enum details and possible values.
     */
    Result<CANMode> ConfigFrame::get_can_mode() const {
        return Result<CANMode>::success(from_byte<CANMode>(
            storage_[to_size_t(ConfigCommandIndex::CAN_MODE)]
        ));
    }
    /**
     * @brief Set the CAN mode in the config frame.
     * Validates the mode before setting.
     * @see common.hpp for CANMode enum details and possible values.
     *
     * @param mode
     * @return Result<Status>
     */
    Result<Status> ConfigFrame::set_can_mode(const CANMode& mode) {
        auto res = validate_can_mode(mode);
        if (res.fail()) {
            return res;
        }
        storage_[to_size_t(ConfigCommandIndex::CAN_MODE)] =
            std::byte{static_cast<uint8_t>(mode)};
        dirty_   = true;
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Get the CAN ID filter from the config frame.
     * @see common.hpp for details on the position of filter bytes.
     * The filter is a 4-byte value.
     * @return Result<FilterType>
     */
    Result<ConfigFrame::FilterType> ConfigFrame::get_id_filter() const {
        FilterType filter;
        std::copy_n(
            storage_.begin() + to_size_t(ConfigCommandIndex::FILTER_ID_1),
            4,
            filter.begin()
        );
        return Result<FilterType>::success(filter);
    }
    /**
     * @brief Set the CAN ID filter in the config frame.
     * Validates the filter before setting.
     * The filter is a 4-byte value.
     * @param filter
     * @return Result<Status>
     */
    Result<Status> ConfigFrame::set_id_filter(const FilterType& filter) {
        auto res = validate_id_filter(filter);
        if (res.fail()) {
            return res;
        }
        std::copy_n(
            filter.begin(),
            4,
            storage_.begin() + to_size_t(ConfigCommandIndex::FILTER_ID_1)
        );
        dirty_   = true;
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Get the CAN ID mask from the config frame.
     * The mask is a 4-byte value.
     */
    Result<ConfigFrame::MaskType> ConfigFrame::get_id_mask() const {
        MaskType mask;
        std::copy_n(
            storage_.begin() + to_size_t(ConfigCommandIndex::MASK_ID_1),
            4,
            mask.begin()
        );
        return Result<MaskType>::success(mask);
    }
    /**
     * @brief Set the CAN ID mask in the config frame.
     * Validates the mask before setting.
     * The mask is a 4-byte value.
     * @param mask
     * @return Result<Status>
     */
    Result<Status> ConfigFrame::set_id_mask(const MaskType& mask) {
        auto res = validate_id_mask(mask);
        if (res.fail()) {
            return res;
        }
        std::copy_n(
            mask.begin(),
            4,
            storage_.begin() + to_size_t(ConfigCommandIndex::MASK_ID_1)
        );
        dirty_   = true;
        return Result<Status>::success(Status::SUCCESS);
    }
    // * Validation methods
    /**
     * @brief Validate the entire config frame.
     * Checks start byte, header byte, type, frame type, baud rate, CAN mode, filter, mask, and checksum.
     *
     * @return Result<Status>
     */
    Result<Status> ConfigFrame::impl_validate() const {
        // Validate start byte
        auto res = validate_start_byte();
        if (res.fail()) {
            return res;
        }
        // Validate header byte
        res = validate_header_byte();
        if (res.fail()) {
            return res;
        }
        // Validate type
        Type type = impl_get_type().value;
        res = validate_type(type);
        if (res.fail()) {
            return res;
        }
        // Validate frame type
        FrameType frame_type = impl_get_frame_type().value;
        res = validate_frame_type(frame_type);
        if (res.fail()) {
            return res;
        }
        // Validate baud rate
        CANBaud baud_rate = get_baud_rate().value;
        res = validate_baud_rate(baud_rate);
        if (res.fail()) {
            return res;
        }
        // Validate CAN mode
        CANMode can_mode = get_can_mode().value;
        res = validate_can_mode(can_mode);
        if (res.fail()) {
            return res;
        }
        // Validate ID filter
        FilterType filter = get_id_filter().value;
        res = validate_id_filter(filter);
        if (res.fail()) {
            return res;
        }
        // Validate ID mask
        MaskType mask = get_id_mask().value;
        res = validate_id_mask(mask);
        if (res.fail()) {
            return res;
        }
        // Validate reserved bytes (BACKUP_0 to BACKUP_3)
        res = validate_reserved_byte();
        if (res.fail()) {
            return res;
        }
        // Validate checksum
        res = validateChecksum(*this);
        if (res.fail()) {
            return res;
        }

        return Result<Status>::success(Status::SUCCESS);
    }

    // ? Individual field validation methods
    /**
     * @brief Validate the start byte of the config frame.
     * Must be equal to Constants::START_BYTE.
     * @return Result<Status> SUCCESS if valid, WBAD_START otherwise.
     */
    Result<Status> ConfigFrame::validate_start_byte() const {
        if (storage_[to_size_t(ConfigCommandIndex::START)] != to_byte(Constants::START_BYTE)) {
            return Result<Status>::error(Status::WBAD_START);
        }
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Validate the header byte of the config frame.
     * Must be equal to Constants::MSG_HEADER.
     * @return Result<Status> SUCCESS if valid, WBAD_HEADER otherwise.
     */
    Result<Status> ConfigFrame::validate_header_byte() const {
        if (storage_[to_size_t(ConfigCommandIndex::HEADER)] != to_byte(Constants::MSG_HEADER)) {
            return Result<Status>::error(Status::WBAD_HEADER);
        }
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Validate the type field of the config frame.
     * Valid types are Type::CONF_FIXED and Type::CONF_VAR.
     * @param type
     * @return Result<Status> SUCCESS if valid, WBAD_TYPE otherwise.
     */
    Result<Status> ConfigFrame::validate_type(const Type& type) const {
        if (type != Type::CONF_FIXED && type != Type::CONF_VAR) {
            return Result<Status>::error(Status::WBAD_TYPE);
        }
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Validate the frame type field of the config frame.
     * Valid frame types are FrameType::STD_FIXED, FrameType::EXT_FIXED.
     * @param frame_type
     * @return Result<Status> SUCCESS if valid, WBAD_FRAME_TYPE otherwise.
     */
    Result<Status> ConfigFrame::validate_frame_type(const FrameType& frame_type) const {
        if (frame_type != FrameType::STD_FIXED && frame_type != FrameType::STD_VAR) {
            return Result<Status>::error(Status::WBAD_FRAME_TYPE);
        }
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Validate the filter mask field of the config frame.
     * The filter mask should be compliant to the selected frame type.
     * For STD frames, the filter must be within 11 bits (0x000 to 0x7FF).
     * For EXT frames, the filter must be within 29 bits (0x00000000 to 0x1FFFFFFF).
     * So the if there's a bit set outside these ranges, it's invalid.
     * @param filter
     * @return Result<Status> SUCCESS if valid, WBAD_ID otherwise.
     */
    Result<Status> ConfigFrame::validate_id_filter(const FilterType& filter) const {
        FrameType frame_type = impl_get_frame_type().value;
        uint32_t filter_value = (static_cast<uint32_t>(filter[0]) << 24) |
            (static_cast<uint32_t>(filter[1]) << 16) |
            (static_cast<uint32_t>(filter[2]) << 8) |
            (static_cast<uint32_t>(filter[3]));
        if (frame_type == FrameType::STD_FIXED) {
            // Standard frame: filter must be within 11 bits
            if (filter_value > 0x7FF) {
                return Result<Status>::error(Status::WBAD_FILTER);
            }
        } else if (frame_type == FrameType::EXT_FIXED) {
            // Extended frame: filter must be within 29 bits
            if (filter_value > 0x1FFFFFFF) {
                return Result<Status>::error(Status::WBAD_FILTER);
            }
        } else {
            // Invalid frame type for filter validation
            return Result<Status>::error(Status::WBAD_FRAME_TYPE);
        }
        return Result<Status>::success(Status::SUCCESS);
    }
    /**
     * @brief Validate the mask field of the config frame.
     * The mask should be compliant to the selected frame type.
     * For STD frames, the mask must be within 11 bits (0x000 to 0x7FF).
     * For EXT frames, the mask must be within 29 bits (0x00000000 to 0x1FFFFFFF).
     * So the if there's a bit set outside these ranges, it's invalid.
     * @param mask
     * @return Result<Status> SUCCESS if valid, WBAD_ID otherwise.
     */
    Result<Status> ConfigFrame::validate_id_mask(const MaskType& mask) const {
        FrameType frame_type = impl_get_frame_type().value;
        uint32_t mask_value = (static_cast<uint32_t>(mask[0]) << 24) |
            (static_cast<uint32_t>(mask[1]) << 16) |
            (static_cast<uint32_t>(mask[2]) << 8) |
            (static_cast<uint32_t>(mask[3]));
        if (frame_type == FrameType::STD_FIXED) {
            // Standard frame: mask must be within 11 bits
            if (mask_value > 0x7FF) {
                return Result<Status>::error(Status::WBAD_MASK);
            }
        } else if (frame_type == FrameType::EXT_FIXED) {
            // Extended frame: mask must be within 29 bits
            if (mask_value > 0x1FFFFFFF) {
                return Result<Status>::error(Status::WBAD_MASK);
            }
        } else {
            // Invalid frame type for mask validation
            return Result<Status>::error(Status::WBAD_FRAME_TYPE);
        }
        return Result<Status>::success(Status::SUCCESS);
    }

    /**
     * @brief Validate the CAN baud rate field of the config frame.
     * The baud rate must be one of the defined CANBaud enum values.
     * @param baud_rate
     * @return Result<Status> SUCCESS if valid, WBAD_CAN_BAUD otherwise.
     */
    Result<Status> ConfigFrame::validate_baud_rate(const CANBaud& baud_rate) const {
        switch (baud_rate) {
        case CANBaud::SPEED_10K:
        case CANBaud::SPEED_20K:
        case CANBaud::SPEED_50K:
        case CANBaud::SPEED_100K:
        case CANBaud::SPEED_125K:
        case CANBaud::SPEED_250K:
        case CANBaud::SPEED_500K:
        case CANBaud::SPEED_800K:
        case CANBaud::SPEED_1000K:
            return Result<Status>::success(Status::SUCCESS);
        default:
            return Result<Status>::error(Status::WBAD_CAN_BAUD);
        }
    }
    /**
     * @brief Validate the CAN mode field of the config frame.
     * The CAN mode must be one of the defined CANMode enum values.
     * @param mode
     * @return Result<Status> SUCCESS if valid, WBAD_CAN_MODE otherwise.
     */
    Result<Status> ConfigFrame::validate_can_mode(const CANMode& mode) const {
        switch (mode) {
        case CANMode::NORMAL:
        case CANMode::SILENT:
        case CANMode::LOOPBACK:
        case CANMode::LOOPBACK_SILENT:
            return Result<Status>::success(Status::SUCCESS);
        default:
            return Result<Status>::error(Status::WBAD_CAN_MODE);
        }
    }
    /**
     * @brief Validate the reserved bytes (BACKUP_0 to BACKUP_3) in the config frame.
     * These bytes should always be zero.
     * @return Result<Status> SUCCESS if valid, WBAD_RESERVED otherwise.
     */
    Result<Status> ConfigFrame::validate_reserved_byte() const {
        for (std::size_t i = to_size_t(ConfigCommandIndex::BACKUP_0);
            i <= to_size_t(ConfigCommandIndex::BACKUP_3);
            i++) {
            if (storage_[i] != std::byte{0x00}) {
                return Result<Status>::error(Status::WBAD_RESERVED);
            }
        }
        return Result<Status>::success(Status::SUCCESS);
    }

}
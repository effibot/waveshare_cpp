/**
 * @file config_frame.cpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief Implementation of ConfigFrame methods.
 * @version 0.1
 * @date 2025-10-03
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "../include/frame/config_frame.hpp"


namespace USBCANBridge {

    namespace CONFIG {
        constexpr size_t _START_ = ConfigFrame::layout::START_OFFSET;
        constexpr size_t _HEADER_ = ConfigFrame::layout::HEADER_OFFSET;
        constexpr size_t _TYPE_ = ConfigFrame::layout::TYPE_OFFSET;
        constexpr size_t _BAUD_ = ConfigFrame::layout::BAUD_OFFSET;
        constexpr size_t _FRAME_TYPE_ = ConfigFrame::layout::FRAME_TYPE_OFFSET;
        constexpr size_t _MODE_ = ConfigFrame::layout::MODE_OFFSET;
        constexpr size_t _FILTER_ = ConfigFrame::layout::FILTER_OFFSET;
        constexpr size_t _MASK_ = ConfigFrame::layout::MASK_OFFSET;
        constexpr size_t _RESERVED_ = ConfigFrame::layout::RESERVED_OFFSET;
        constexpr size_t _RESERVED_SIZE_ = ConfigFrame::layout::RESERVED_SIZE;
        constexpr size_t _CHECKSUM_ = ConfigFrame::layout::CHECKSUM_OFFSET;
    }

    // === Core impl_*() Methods ===

    Result<void> ConfigFrame::impl_deserialize(span<const std::byte> data) {
        // Check size
        if (data.size() != traits::FRAME_SIZE) {
            return Result<void>::error(Status::WBAD_LENGTH,
                "ConfigFrame::impl_deserialize");
        }
        // Copy data into internal storage
        std::copy(data.begin(), data.end(), frame_storage_.begin());
        // Mark checksum as dirty since we changed the frame
        checksum_interface_.mark_dirty();
        return Result<void>::success();
    }

    // === ConfigFrame impl_*() Methods ===
    Result<FrameType> ConfigFrame::impl_get_frame_type() const {
        std::byte frame_type = frame_storage_[CONFIG::_FRAME_TYPE_];
        return Result<FrameType>::success(from_byte<FrameType>(frame_type));
    }
    Result<void> ConfigFrame::impl_set_frame_type(FrameType type) {
        frame_storage_[CONFIG::_FRAME_TYPE_] = to_byte(type);
        // Mark checksum as dirty since we changed the frame
        checksum_interface_.mark_dirty();
        return Result<void>::success();
    }
    Result<CANBaud> ConfigFrame::impl_get_baud_rate() const {
        std::byte baud = frame_storage_[CONFIG::_BAUD_];
        return Result<CANBaud>::success(from_byte<CANBaud>(baud));
    }
    Result<void> ConfigFrame::impl_set_baud_rate(CANBaud baud_rate) {
        frame_storage_[CONFIG::_BAUD_] = to_byte(baud_rate);
        // Mark checksum as dirty since we changed the frame
        checksum_interface_.mark_dirty();
        return Result<void>::success();
    }
    Result<CANMode> ConfigFrame::impl_get_can_mode() const {
        std::byte mode = frame_storage_[CONFIG::_MODE_];
        return Result<CANMode>::success(from_byte<CANMode>(mode));
    }
    Result<void> ConfigFrame::impl_set_can_mode(CANMode mode) {
        frame_storage_[CONFIG::_MODE_] = to_byte(mode);
        // Mark checksum as dirty since we changed the frame
        checksum_interface_.mark_dirty();
        return Result<void>::success();
    }
    Result<uint32_t> ConfigFrame::impl_get_filter() const {
        uint32_t filter = 0;
        for (size_t i = 0; i < 4; ++i) {
            filter = (filter << 8) | static_cast<uint8_t>(frame_storage_[CONFIG::_FILTER_ + i]);
        }
        return Result<uint32_t>::success(filter);
    }
    Result<void> ConfigFrame::impl_set_filter(uint32_t filter) {
        for (size_t i = 0; i < 4; ++i) {
            frame_storage_[CONFIG::_FILTER_ + 3 - i] = (static_cast<std::byte>(filter & 0xFF));
            filter >>= 8;
        }
        // Mark checksum as dirty since we changed the frame
        checksum_interface_.mark_dirty();
        return Result<void>::success();
    }
    Result<uint32_t> ConfigFrame::impl_get_mask() const {
        uint32_t mask = 0;
        for (size_t i = 0; i < 4; ++i) {
            mask = (mask << 8) | static_cast<uint8_t>(frame_storage_[CONFIG::_MASK_ + i]);
        }
        return Result<uint32_t>::success(mask);
    }
    Result<void> ConfigFrame::impl_set_mask(uint32_t mask) {
        for (size_t i = 0; i < 4; ++i) {
            frame_storage_[CONFIG::_MASK_ + 3 - i] = (static_cast<std::byte>(mask & 0xFF));
            mask >>= 8;
        }
        // Mark checksum as dirty since we changed the frame
        checksum_interface_.mark_dirty();
        return Result<void>::success();
    }
    Result<RTX> ConfigFrame::impl_get_auto_rtx() const {
        std::byte rtx = frame_storage_[CONFIG::_RESERVED_ + CONFIG::_RESERVED_SIZE_ - 1];
        return Result<RTX>::success(from_byte<RTX>(rtx));
    }

    Result<void> ConfigFrame::impl_set_auto_rtx(RTX rtx) {
        frame_storage_[CONFIG::_RESERVED_ + CONFIG::_RESERVED_SIZE_ - 1] = to_byte(rtx);
        // Mark checksum as dirty since we changed the frame
        checksum_interface_.mark_dirty();
        return Result<void>::success();
    }


} // namespace USBCANBridge
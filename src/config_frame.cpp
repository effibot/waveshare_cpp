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

    // === ConfigFrame impl_*() Methods ===
    Type ConfigFrame::impl_get_type() const {
        std::byte frame_type = frame_storage_[layout_.TYPE];
        return from_byte<Type>(frame_type);
    }
    void ConfigFrame::impl_set_type(Type type) {
        frame_storage_[layout_.TYPE] = to_byte(type);
        // Mark checksum as dirty since we changed the frame
        checksum_interface_.mark_dirty();
    }

    CANVersion ConfigFrame::impl_get_CAN_version() const {
        std::byte frame_type = frame_storage_[layout_.CAN_VERS];
        return from_byte<CANVersion>(frame_type);
    }
    void ConfigFrame::impl_set_CAN_version(CANVersion type) {
        frame_storage_[layout_.CAN_VERS] = to_byte(type);
        // Mark checksum as dirty since we changed the frame
        checksum_interface_.mark_dirty();
    }

} // namespace USBCANBridge
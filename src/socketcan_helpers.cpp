/**
 * @file socketcan_helpers.cpp
 * @brief SocketCAN conversion utilities implementation
 * @version 1.0
 * @date 2025-10-14
 */

#include <cstring>  // For memset

#include "../include/interface/socketcan_helpers.hpp"
#include "../include/pattern/frame_builder.hpp"

namespace waveshare {

    // === to_socketcan() Implementation ===

    struct can_frame SocketCANHelper::to_socketcan(const VariableFrame& frame) {
        struct can_frame cf;
        std::memset(&cf, 0, sizeof(cf));  // Zero-initialize

        // Extract CAN ID and determine if extended
        std::uint32_t can_id = frame.get_can_id();
        bool is_extended = frame.is_extended();
        bool is_remote = (frame.get_format() == Format::REMOTE_VARIABLE);

        // Set can_id with appropriate flags
        cf.can_id = can_id;
        if (is_extended) {
            cf.can_id |= CAN_EFF_FLAG;  // Mark as extended ID
        }
        if (is_remote) {
            cf.can_id |= CAN_RTR_FLAG;  // Mark as remote frame
        }

        // Set DLC
        std::uint8_t dlc = frame.get_dlc();
        if (dlc > 8) {
            throw ProtocolException(Status::WBAD_DLC,
                "to_socketcan: DLC must be 0-8, got " + std::to_string(dlc));
        }
        cf.can_dlc = dlc;

        // Copy data bytes (only for data frames)
        if (!is_remote) {
            auto data = frame.get_data();
            if (data.size() != dlc) {
                throw ProtocolException(Status::WBAD_LENGTH,
                    "to_socketcan: data size mismatch (DLC=" + std::to_string(dlc) +
                    ", data.size()=" + std::to_string(data.size()) + ")");
            }
            std::copy(data.begin(), data.end(), cf.data);
        }

        return cf;
    }

    // === from_socketcan() Implementation ===

    VariableFrame SocketCANHelper::from_socketcan(const struct can_frame& cf) {
        // Extract flags from can_id
        bool is_extended = (cf.can_id & CAN_EFF_FLAG) != 0;
        bool is_remote = (cf.can_id & CAN_RTR_FLAG) != 0;

        // Extract actual CAN ID (mask off flags)
        std::uint32_t can_id;
        if (is_extended) {
            can_id = cf.can_id & CAN_EFF_MASK;  // 29-bit mask
        } else {
            can_id = cf.can_id & CAN_SFF_MASK;  // 11-bit mask
        }

        // Determine CANVersion and Format
        CANVersion version = is_extended ? CANVersion::EXT_VARIABLE : CANVersion::STD_VARIABLE;
        Format format = is_remote ? Format::REMOTE_VARIABLE : Format::DATA_VARIABLE;

        // Validate DLC
        if (cf.can_dlc > 8) {
            throw ProtocolException(Status::WBAD_DLC,
                "from_socketcan: can_dlc must be 0-8, got " + std::to_string(cf.can_dlc));
        }

        // Build VariableFrame using FrameBuilder
        // Note: For remote frames, DLC in SocketCAN indicates requested length,
        // but VariableFrame currently only supports DLC=data.size()
        // Remote frames will have DLC=0 for now
        auto builder = FrameBuilder<VariableFrame>()
            .with_type(version, format)
            .with_id(can_id);

        // Add data bytes (only for data frames)
        if (!is_remote && cf.can_dlc > 0) {
            std::vector<std::uint8_t> data(cf.data, cf.data + cf.can_dlc);
            builder = builder.with_data(data);
        }

        // Build and return (builder will validate)
        return std::move(builder).build();
    }

} // namespace waveshare

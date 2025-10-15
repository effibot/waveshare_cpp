/**
 * @file socketcan_helpers.hpp
 * @brief SocketCAN conversion utilities for Waveshare frames
 * @version 1.0
 * @date 2025-10-14
 *
 * Provides bidirectional conversion between Waveshare VariableFrame
 * and Linux SocketCAN struct can_frame.
 *
 * Conversion Rules:
 * - VariableFrame ↔ struct can_frame
 * - Standard ID (11-bit) vs Extended ID (29-bit) via CAN_EFF_FLAG
 * - Remote frames via CAN_RTR_FLAG
 * - Data frames with DLC 0-8 bytes
 *
 * @copyright Copyright (c) 2025
 */

#pragma once

#include <linux/can.h>
#include <linux/can/raw.h>

#include "../frame/variable_frame.hpp"
#include "../exception/waveshare_exception.hpp"

namespace waveshare {
    /**
     * @brief Static utility class for SocketCAN conversions
     *
     * Provides methods to convert between Waveshare VariableFrame
     * and Linux SocketCAN can_frame structures.
     *
     * All methods are static - no instances required.
     */
    class SocketCANHelper {
        public:
            /**
             * @brief Convert VariableFrame to SocketCAN can_frame
             *
             * Conversion:
             * - CAN ID → can_id (with CAN_EFF_FLAG for extended IDs)
             * - Format → CAN_RTR_FLAG (for remote frames)
             * - DLC → can_dlc
             * - Data → data[0..7]
             *
             * @param frame Waveshare VariableFrame to convert
             * @return struct can_frame Linux SocketCAN frame
             * @throws ProtocolException if frame is invalid
             */
            static struct can_frame to_socketcan(const VariableFrame& frame);

            /**
             * @brief Convert SocketCAN can_frame to VariableFrame
             *
             * Conversion:
             * - can_id → CAN ID (extracts CAN_EFF_FLAG, CAN_RTR_FLAG)
             * - CAN_EFF_FLAG → CANVersion (STD_VARIABLE or EXT_VARIABLE)
             * - CAN_RTR_FLAG → Format (REMOTE_VARIABLE or DATA_VARIABLE)
             * - can_dlc → DLC
             * - data[0..7] → Data bytes
             *
             * @param cf Linux SocketCAN frame to convert
             * @return VariableFrame Waveshare frame
             * @throws ProtocolException if can_frame is invalid
             */
            static VariableFrame from_socketcan(const struct can_frame& cf);

        private:
            // No instances allowed
            SocketCANHelper() = delete;
            ~SocketCANHelper() = delete;
            SocketCANHelper(const SocketCANHelper&) = delete;
            SocketCANHelper& operator=(const SocketCANHelper&) = delete;
    };

} // namespace waveshare

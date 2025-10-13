/**
 * @file frames.hpp
 * @author effibot (andrea.efficace1@gmail.com)
 * @brief Header file to facilitate the inclusion of the Waveshare Adapter library
 * @version 0.1
 * @date 2025-10-13
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

// Include the protocol enums
#include "enums/protocol.hpp"
// Include the result template
#include "template/result.hpp"
// Include the frame traits
#include "template/frame_traits.hpp"
// Include the frame interfaces
#include "interface/core.hpp"
#include "interface/serialization_helpers.hpp"
#include "interface/config.hpp"
#include "interface/data.hpp"
// Include the frame implementations
#include "frame/config_frame.hpp"
#include "frame/fixed_frame.hpp"
#include "frame/variable_frame.hpp"
// Include the frame builders
#include "pattern/frame_builder.hpp"
// Include the USB adapter interface
#include "pattern/usb_adapter.hpp"


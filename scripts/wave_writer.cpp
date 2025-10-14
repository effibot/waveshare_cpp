/**
 * @file wave_writer.cpp
 * @author effibot (andrea.efficace1@gmail.com)
 * @brief Simple example to write frames to the Waveshare USB-CAN adapter
 * @version 0.1
 * @date 2025-10-13
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "script_utils.hpp"
#include <chrono>
#include <thread>

using namespace USBCANBridge;

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    auto config_result = parse_arguments(argc, argv, false);
    if (config_result.fail()) {
        return 1;
    }
    ScriptConfig config = config_result.value();

    // Initialize and configure adapter
    auto adapter_result = initialize_adapter(config);
    if (adapter_result.fail()) {
        return 1;
    }
    USBAdapter* adapter = adapter_result.value();

    // Create data frame based on configuration
    std::variant<FixedFrame, VariableFrame> data_frame;

    if (config.use_fixed_frames) {
        data_frame = make_fixed_frame()
            .with_can_version(CANVersion::STD_FIXED)
            .with_id(0x123)
            .with_data({0xDE, 0xAD, 0xBE, 0xEF})
            .build();
    } else {
        data_frame = make_variable_frame()
            .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
            .with_id(0x123)
            .with_data({0xDE, 0xAD, 0xBE, 0xEF})
            .build();
    }

    // Send frames in a loop
    for (int i = 0; i < 10; ++i) {
        Result<void> send_result;

        if (std::holds_alternative<FixedFrame>(data_frame)) {
            const auto& frame = std::get<FixedFrame>(data_frame);
            send_result = adapter->send_frame(frame);
            if (send_result.success()) {
                std::cout << "Sent >>\t\t" << frame.to_string() << "\n";
            }
        } else if (std::holds_alternative<VariableFrame>(data_frame)) {
            const auto& frame = std::get<VariableFrame>(data_frame);
            send_result = adapter->send_frame(frame);
            if (send_result.success()) {
                std::cout << "Sent >>\t\t" << frame.to_string() << "\n";
            }
        }

        if (send_result.fail()) {
            std::cerr << "Error sending data frame: " << send_result.describe() << "\n";
            break;
        }

        // Wait before sending next frame
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Cleanup
    delete adapter;
    return 0;
}
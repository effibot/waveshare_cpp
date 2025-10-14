/**
 * @file wave_reader.cpp
 * @author effibot (andrea.efficace1@gmail.com)
 * @brief Simple example to read frames from the Waveshare USB-CAN adapter
 * @version 0.1
 * @date 2025-10-13
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "script_utils.hpp"

using namespace USBCANBridge;

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    auto config_result = parse_arguments(argc, argv, true);
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

    // Read frames in a loop
    while (!USBAdapter::should_stop()) {
        std::string frame_string;

        if (config.use_fixed_frames) {
            auto frame_res = adapter->receive_fixed_frame(1000); // 1 second timeout
            if (frame_res.fail()) {
                if (frame_res.error() == Status::WTIMEOUT) {
                    continue; // Timeout, no frame received
                }
                std::cerr << "Failed to receive frame: " << frame_res.describe() << "\n";
                break;
            }
            frame_string = frame_res.value().to_string();
        } else {
            auto frame_res = adapter->receive_variable_frame(1000); // 1 second timeout
            if (frame_res.fail()) {
                if (frame_res.error() == Status::WTIMEOUT) {
                    continue; // Timeout, no frame received
                }
                std::cerr << "Failed to receive frame: " << frame_res.describe() << "\n";
                break;
            }
            frame_string = frame_res.value().to_string();
        }

        // Process the received frame
        std::cout << "Received <<\t" << frame_string << "\n";
    }

    // Cleanup
    delete adapter;
    return 0;
}
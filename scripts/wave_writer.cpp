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
#include <variant>

using namespace waveshare;

int main(int argc, char* argv[]) {
    try {
        // Parse command-line arguments
        ScriptConfig config = parse_arguments(argc, argv, false);

        // Initialize and configure adapter
        auto adapter = initialize_adapter(config);

        // Create data frame based on configuration
        std::variant<FixedFrame, VariableFrame> data_frame;

        if (config.use_fixed_frames) {
            data_frame = make_fixed_frame()
                .with_can_version(CANVersion::STD_FIXED)
                .with_id(0x0)
                .with_data({0xDE, 0xAD, 0xBE, 0xEF})
                .build();
        } else {
            data_frame = make_variable_frame()
                .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
                .with_id(0x0)
                .with_data({0xDE, 0xAD, 0xBE, 0xEF})
                .build();
        }

        // Send frames in a loop
        for (int i = 0; i < 10; ++i) {
            if (std::holds_alternative<FixedFrame>(data_frame)) {
                std::get<FixedFrame>(data_frame).set_id(0x123 + i); // Change ID each iteration
                const auto& frame = std::get<FixedFrame>(data_frame);
                adapter->send_frame(frame);
                std::cout << "Sent >>\t\t" << frame.to_string() << "\n";
            } else if (std::holds_alternative<VariableFrame>(data_frame)) {
                std::get<VariableFrame>(data_frame).set_id(0x123 + i); // Change ID each iteration
                const auto& frame = std::get<VariableFrame>(data_frame);
                adapter->send_frame(frame);
                std::cout << "Sent >>\t\t" << frame.to_string() << "\n";
            }

            // Wait before sending next frame
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        // Cleanup (automatic with unique_ptr)
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
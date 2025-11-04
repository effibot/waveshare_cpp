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

using namespace waveshare;

int main(int argc, char* argv[]) {
    try {
        // Parse command-line arguments
        ScriptConfig config = parse_arguments(argc, argv, ScriptType::READER);

        // Initialize and configure adapter with RTX OFF for receiving
        auto adapter = initialize_adapter(config, RTX::OFF);

        std::cout << "\n=== CAN Frame Reader ===\n";
        std::cout << "Waiting for CAN frames (Ctrl+C to stop)...\n\n";

        // Read frames in a loop
        while (!USBAdapter::should_stop()) {
            try {
                std::string frame_string;

                if (config.use_fixed_frames) {
                    auto frame = adapter->receive_fixed_frame(1000); // 1 second timeout
                    frame_string = frame.to_string();
                } else {
                    auto frame = adapter->receive_variable_frame(1000); // 1 second timeout
                    frame_string = frame.to_string();
                }

                // Process the received frame
                std::cout << "[" << get_timestamp() << "] Received << " << frame_string << "\n";
                std::cout.flush();
            } catch (const TimeoutException&) {
                // Timeout, no frame received - this is expected, just continue
                continue;
            }
            catch (const WaveshareException& e) {
                std::cerr << "Failed to receive frame: " << e.what() << "\n";
                break;
            }
        }

        std::cout << "\n[READER] Stopped.\n";

        // Cleanup (automatic with unique_ptr)
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
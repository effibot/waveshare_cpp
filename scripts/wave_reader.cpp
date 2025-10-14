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
        ScriptConfig config = parse_arguments(argc, argv, true);

        // Initialize and configure adapter
        USBAdapter* adapter = initialize_adapter(config);

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
                std::cout << "Received <<\t" << frame_string << "\n";
            } catch (const TimeoutException&) {
                // Timeout, no frame received - this is expected, just continue
                continue;
            }
            catch (const WaveshareException& e) {
                std::cerr << "Failed to receive frame: " << e.what() << "\n";
                break;
            }
        }

        // Cleanup
        delete adapter;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
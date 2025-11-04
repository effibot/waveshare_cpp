/**
 * @file wave_writer.cpp
 * @author effibot (andrea.efficace1@gmail.com)
 * @brief Enhanced CAN frame writer for Waveshare USB-CAN adapter
 * @version 0.2
 * @date 2025-11-04
 *
 * Supports multiple sending modes:
 * - Single message
 * - Fixed count
 * - Infinite loop
 * - Incrementing CAN ID
 * - Configurable delay between messages
 *
 * @copyright Copyright (c) 2025
 */

#include "script_utils.hpp"
#include <chrono>
#include <thread>
#include <variant>
#include <csignal>

using namespace waveshare;

// Global flag for signal handling
static volatile bool running = true;

/**
 * @brief Signal handler for graceful shutdown
 */
void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n[SIGNAL] Received SIGINT (Ctrl+C) - stopping...\n";
        running = false;
    }
}

int main(int argc, char* argv[]) {
    try {
        // Install signal handler
        std::signal(SIGINT, signal_handler);

        // Parse command-line arguments
        ScriptConfig config = parse_arguments(argc, argv, ScriptType::WRITER);

        // Initialize and configure adapter
        auto adapter = initialize_adapter(config);

        std::cout << "=== CAN Frame Writer ===\n\n";
        std::cout << "Configuration:\n";
        std::cout << "  Device:          " << config.device << "\n";
        std::cout << "  Serial Baud:     " << static_cast<int>(config.serial_baudrate) << " bps\n";
        std::cout << "  CAN Baud:        " << static_cast<int>(config.can_baudrate) << " bps\n";
        std::cout << "  Frame Type:      " << (config.use_fixed_frames ? "Fixed" : "Variable") <<
            "\n";
        std::cout << "  Start ID:        0x" << std::hex << std::uppercase << std::setfill('0')
                  << std::setw(config.start_id >
            0x7FF ? 8 : 3) << config.start_id << std::dec << "\n";
        std::cout << "  Data:            " << format_can_data(config.message_data.data(),
            config.message_data.size()) << "\n";
        std::cout << "  Message Gap:     " << config.message_gap_ms << " ms\n";
        std::cout << "  Increment ID:    " << (config.increment_id ? "Yes" : "No") << "\n";

        if (config.writer_mode == WriterMode::LOOP || config.message_count == 0) {
            std::cout << "  Mode:            Infinite loop (press Ctrl+C to stop)\n";
        } else {
            std::cout << "  Mode:            Send " << config.message_count << " messages\n";
        }
        std::cout << "\n";

        // Create data frame based on configuration
        std::variant<FixedFrame, VariableFrame> data_frame;
        std::uint32_t current_id = config.start_id;
        std::uint32_t messages_sent = 0;

        // Determine if we should use extended ID format
        bool is_extended = (current_id > 0x7FF);

        if (config.use_fixed_frames) {
            data_frame = make_fixed_frame()
                .with_can_version(is_extended ? CANVersion::EXT_FIXED : CANVersion::STD_FIXED)
                .with_format(Format::DATA_FIXED)
                .with_id(current_id)
                .with_data(config.message_data)
                .build();
        } else {
            data_frame = make_variable_frame()
                .with_type(is_extended ? CANVersion::EXT_VARIABLE : CANVersion::STD_VARIABLE,
                Format::DATA_VARIABLE)
                .with_id(current_id)
                .with_data(config.message_data)
                .build();
        }

        std::cout << "Starting transmission...\n\n";

        // Main sending loop
        while (running) {
            // Check if we've reached the message count (if not in loop mode)
            if (config.writer_mode != WriterMode::LOOP && config.message_count > 0) {
                if (messages_sent >= config.message_count) {
                    break;
                }
            }

            // Update frame ID
            if (std::holds_alternative<FixedFrame>(data_frame)) {
                std::get<FixedFrame>(data_frame).set_id(current_id);
                const auto& frame = std::get<FixedFrame>(data_frame);
                adapter->send_frame(frame);
                std::cout << "[" << get_timestamp() << "] Sent >> " << frame.to_string() << "\n";
            } else if (std::holds_alternative<VariableFrame>(data_frame)) {
                std::get<VariableFrame>(data_frame).set_id(current_id);
                const auto& frame = std::get<VariableFrame>(data_frame);
                adapter->send_frame(frame);
                std::cout << "[" << get_timestamp() << "] Sent >> " << frame.to_string() << "\n";
            }

            messages_sent++;

            // Increment ID if requested
            if (config.increment_id) {
                current_id++;
                // Wrap around at max ID for standard/extended frames
                if (is_extended && current_id > 0x1FFFFFFF) {
                    current_id = config.start_id;
                } else if (!is_extended && current_id > 0x7FF) {
                    current_id = config.start_id;
                }
            }

            // Wait before sending next frame (if not the last message)
            bool is_last_message = (config.writer_mode != WriterMode::LOOP &&
                config.message_count > 0 && messages_sent >= config.message_count);

            if (!is_last_message && config.message_gap_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(config.message_gap_ms));
            }
        }

        std::cout << "\n=== Transmission Complete ===\n";
        std::cout << "Total messages sent: " << messages_sent << "\n";

        return 0;
    } catch (const WaveshareException& e) {
        std::cerr << "\n[ERROR] Waveshare error: " << e.what() << "\n";
        std::cerr << "  Status code: " << static_cast<int>(e.status()) << "\n";
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "\n[ERROR] " << e.what() << "\n";
        return 1;
    }
}
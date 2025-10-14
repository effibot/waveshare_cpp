/**
 * @file script_utils.hpp
 * @author effibot (andrea.efficace1@gmail.com)
 * @brief Shared utilities for Waveshare USB-CAN scripts
 * @version 0.1
 * @date 2025-10-14
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include "../include/waveshare.hpp"
#include <string>
#include <iostream>
#include <getopt.h>

namespace waveshare {

// === Command-Line Argument Parsing ===

/**
 * @brief Program configuration structure
 */
    struct ScriptConfig {
        std::string device = "/dev/ttyUSB0";
        SerialBaud serial_baudrate = SerialBaud::BAUD_2M;
        CANBaud can_baudrate = CANBaud::BAUD_1M;
        bool use_fixed_frames = false;
    };

/**
 * @brief Display help message for script usage
 * @param program_name The name of the program (argv[0])
 * @param is_reader True for reader script, false for writer script
 */
    inline void display_help(const std::string& program_name, bool is_reader) {
        std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
        std::cout << "Options:\n";
        std::cout << "  -d <device>    Serial device path (default: /dev/ttyUSB0)\n";
        std::cout << "  -s <baudrate>  Serial baudrate (default: 2000000)\n";
        std::cout <<
            "                 Supported: 9600, 19200, 38400, 57600, 115200, 153600, 2000000\n";
        std::cout << "  -b <baudrate>  CAN bus baudrate (default: 1000000)\n";
        std::cout << "                 Supported: 10000, 20000, 50000, 100000, 125000, 200000,\n";
        std::cout << "                            250000, 400000, 500000, 800000, 1000000\n";
        std::cout << "  -f <type>      Frame type: 'fixed' or 'variable' (default: variable)\n";
        std::cout << "  -h             Display this help message\n";
        std::cout << "\n";
        if (is_reader) {
            std::cout << "Reads CAN frames from the Waveshare USB-CAN adapter and displays them.\n";
        } else {
            std::cout << "Sends test CAN frames to the Waveshare USB-CAN adapter.\n";
        }
    }

/**
 * @brief Parse serial baudrate from string
 * @param baud_str String representation of baudrate
 * @return Result<SerialBaud> Parsed baudrate or error
 */
    inline Result<SerialBaud> parse_serial_baudrate(const std::string& baud_str) {
        try {
            int baud = std::stoi(baud_str);
            switch (baud) {
            case 9600:    return Result<SerialBaud>::success(SerialBaud::BAUD_9600);
            case 19200:   return Result<SerialBaud>::success(SerialBaud::BAUD_19200);
            case 38400:   return Result<SerialBaud>::success(SerialBaud::BAUD_38400);
            case 57600:   return Result<SerialBaud>::success(SerialBaud::BAUD_57600);
            case 115200:  return Result<SerialBaud>::success(SerialBaud::BAUD_115200);
            case 153600:  return Result<SerialBaud>::success(SerialBaud::BAUD_153600);
            case 2000000: return Result<SerialBaud>::success(SerialBaud::BAUD_2M);
            default:
                return Result<SerialBaud>::error(Status::UNKNOWN, "parse_serial_baudrate");
            }
        } catch (const std::invalid_argument&) {
            return Result<SerialBaud>::error(Status::UNKNOWN, "parse_serial_baudrate");
        }
    }

/**
 * @brief Parse CAN baudrate from string
 * @param baud_str String representation of baudrate
 * @return Result<CANBaud> Parsed baudrate or error
 */
    inline Result<CANBaud> parse_can_baudrate(const std::string& baud_str) {
        try {
            int baud = std::stoi(baud_str);
            switch (baud) {
            case 10000:   return Result<CANBaud>::success(CANBaud::BAUD_10K);
            case 20000:   return Result<CANBaud>::success(CANBaud::BAUD_20K);
            case 50000:   return Result<CANBaud>::success(CANBaud::BAUD_50K);
            case 100000:  return Result<CANBaud>::success(CANBaud::BAUD_100K);
            case 125000:  return Result<CANBaud>::success(CANBaud::BAUD_125K);
            case 200000:  return Result<CANBaud>::success(CANBaud::BAUD_200K);
            case 250000:  return Result<CANBaud>::success(CANBaud::BAUD_250K);
            case 400000:  return Result<CANBaud>::success(CANBaud::BAUD_400K);
            case 500000:  return Result<CANBaud>::success(CANBaud::BAUD_500K);
            case 800000:  return Result<CANBaud>::success(CANBaud::BAUD_800K);
            case 1000000: return Result<CANBaud>::success(CANBaud::BAUD_1M);
            default:
                return Result<CANBaud>::error(Status::WBAD_CAN_BAUD, "parse_can_baudrate");
            }
        } catch (const std::invalid_argument&) {
            return Result<CANBaud>::error(Status::WBAD_CAN_BAUD, "parse_can_baudrate");
        }
    }

/**
 * @brief Parse frame type from string
 * @param type_str String representation of frame type ("fixed" or "variable")
 * @return Result<bool> True for fixed, false for variable, or error
 */
    inline Result<bool> parse_frame_type(const std::string& type_str) {
        if (type_str == "fixed") {
            return Result<bool>::success(true);
        } else if (type_str == "variable") {
            return Result<bool>::success(false);
        } else {
            return Result<bool>::error(Status::WBAD_FRAME_TYPE, "parse_frame_type");
        }
    }

/**
 * @brief Parse command-line arguments
 * @param argc Argument count
 * @param argv Argument values
 * @param is_reader True for reader script, false for writer script
 * @return Result<ScriptConfig> Parsed configuration or error
 */
    inline Result<ScriptConfig> parse_arguments(int argc, char* argv[], bool is_reader) {
        ScriptConfig config;
        int opt;

        while ((opt = getopt(argc, argv, "hd:s:b:f:")) != -1) {
            switch (opt) {
            case 'h':
                display_help(argv[0], is_reader);
                std::exit(0);

            case 'd':
                config.device = optarg;
                break;

            case 's': {
                auto result = parse_serial_baudrate(optarg);
                if (result.fail()) {
                    std::cerr << "Invalid serial baudrate: " << optarg << "\n";
                    std::cerr << "Supported: 9600, 19200, 38400, 57600, 115200, 153600, 2000000\n";
                    return Result<ScriptConfig>::error(Status::UNKNOWN, "parse_arguments");
                }
                config.serial_baudrate = result.value();
                break;
            }

            case 'b': {
                auto result = parse_can_baudrate(optarg);
                if (result.fail()) {
                    std::cerr << "Invalid CAN baudrate: " << optarg << "\n";
                    std::cerr <<
                        "Supported: 10000, 20000, 50000, 100000, 125000, 200000, 250000, 400000, 500000, 800000, 1000000\n";
                    return Result<ScriptConfig>::error(Status::WBAD_CAN_BAUD, "parse_arguments");
                }
                config.can_baudrate = result.value();
                break;
            }

            case 'f': {
                auto result = parse_frame_type(optarg);
                if (result.fail()) {
                    std::cerr << "Invalid frame type: " << optarg << "\n";
                    std::cerr << "Use 'fixed' or 'variable'\n";
                    return Result<ScriptConfig>::error(Status::WBAD_FRAME_TYPE, "parse_arguments");
                }
                config.use_fixed_frames = result.value();
                break;
            }

            case '?':
                if (optopt == 'd' || optopt == 's' || optopt == 'b' || optopt == 'f') {
                    std::cerr << "Option -" << static_cast<char>(optopt) <<
                        " requires an argument.\n";
                } else {
                    std::cerr << "Unknown option: -" << static_cast<char>(optopt) << "\n";
                }
                return Result<ScriptConfig>::error(Status::UNKNOWN, "parse_arguments");

            default:
                display_help(argv[0], is_reader);
                return Result<ScriptConfig>::error(Status::UNKNOWN, "parse_arguments");
            }
        }

        return Result<ScriptConfig>::success(config);
    }

// === Adapter Configuration ===

/**
 * @brief Create and configure a ConfigFrame based on script configuration
 * @param config Script configuration
 * @return ConfigFrame Configured frame ready to send
 */
    inline ConfigFrame create_config_frame(const ScriptConfig& config) {
        Type type = config.use_fixed_frames ? Type::CONF_FIXED : Type::CONF_VARIABLE;

        return make_config_frame()
               .with_type(type)
               .with_baud_rate(config.can_baudrate)
               .with_can_version(CANVersion::STD_FIXED)
               .with_filter(0x00000000)
               .with_mask(0x00000000)
               .with_mode(CANMode::NORMAL)
               .with_rtx(RTX::OFF)
               .build();
    }

/**
 * @brief Initialize and configure USB adapter
 * @param config Script configuration
 * @return Result<USBAdapter*> Configured adapter or error
 */
    inline Result<USBAdapter*> initialize_adapter(const ScriptConfig& config) {
        // Create adapter
        USBAdapter* adapter = new USBAdapter(config.device, config.serial_baudrate);

        if (!adapter->is_open()) {
            std::cerr << "Failed to open device: " << config.device << "\n";
            delete adapter;
            return Result<USBAdapter*>::error(Status::DNOT_OPEN, "initialize_adapter");
        }

        std::cout << "Device opened: " << config.device << "\n";
        std::cout << "Serial baudrate: " << static_cast<int>(config.serial_baudrate) << "\n";
        std::cout << "CAN baudrate: " << static_cast<int>(config.can_baudrate) << "\n";
        std::cout << "Frame type: " << (config.use_fixed_frames ? "fixed" : "variable") << "\n";

        // Send configuration frame
        ConfigFrame config_frame = create_config_frame(config);
        auto send_result = adapter->send_frame(config_frame);

        if (send_result.fail()) {
            std::cerr << "Failed to send config frame: " << send_result.describe() << "\n";
            delete adapter;
            return Result<USBAdapter*>::error(send_result.error(), "initialize_adapter");
        }

        std::cout << "Config sent >>\t" << config_frame.to_string() << "\n\n";

        return Result<USBAdapter*>::success(adapter);
    }

} // namespace USBCANBridge

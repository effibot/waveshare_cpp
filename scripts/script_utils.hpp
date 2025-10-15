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
 * @return SerialBaud Parsed baudrate
 * @throws std::invalid_argument if baudrate is not supported
 */
    inline SerialBaud parse_serial_baudrate(const std::string& baud_str) {
        int baud = std::stoi(baud_str);  // May throw std::invalid_argument
        switch (baud) {
        case 9600:    return SerialBaud::BAUD_9600;
        case 19200:   return SerialBaud::BAUD_19200;
        case 38400:   return SerialBaud::BAUD_38400;
        case 57600:   return SerialBaud::BAUD_57600;
        case 115200:  return SerialBaud::BAUD_115200;
        case 153600:  return SerialBaud::BAUD_153600;
        case 2000000: return SerialBaud::BAUD_2M;
        default:
            throw std::invalid_argument("Unsupported serial baudrate: " + baud_str);
        }
    }

/**
 * @brief Parse CAN baudrate from string
 * @param baud_str String representation of baudrate
 * @return CANBaud Parsed baudrate
 * @throws std::invalid_argument if baudrate is not supported
 */
    inline CANBaud parse_can_baudrate(const std::string& baud_str) {
        int baud = std::stoi(baud_str);  // May throw std::invalid_argument
        switch (baud) {
        case 10000:   return CANBaud::BAUD_10K;
        case 20000:   return CANBaud::BAUD_20K;
        case 50000:   return CANBaud::BAUD_50K;
        case 100000:  return CANBaud::BAUD_100K;
        case 125000:  return CANBaud::BAUD_125K;
        case 200000:  return CANBaud::BAUD_200K;
        case 250000:  return CANBaud::BAUD_250K;
        case 400000:  return CANBaud::BAUD_400K;
        case 500000:  return CANBaud::BAUD_500K;
        case 800000:  return CANBaud::BAUD_800K;
        case 1000000: return CANBaud::BAUD_1M;
        default:
            throw std::invalid_argument("Unsupported CAN baudrate: " + baud_str);
        }
    }

/**
 * @brief Parse frame type from string
 * @param type_str String representation of frame type ("fixed" or "variable")
 * @return bool True for fixed, false for variable
 * @throws std::invalid_argument if frame type is invalid
 */
    inline bool parse_frame_type(const std::string& type_str) {
        if (type_str == "fixed") {
            return true;
        } else if (type_str == "variable") {
            return false;
        } else {
            throw std::invalid_argument("Invalid frame type: " + type_str +
                " (use 'fixed' or 'variable')");
        }
    }

/**
 * @brief Parse command-line arguments
 * @param argc Argument count
 * @param argv Argument values
 * @param is_reader True for reader script, false for writer script
 * @return ScriptConfig Parsed configuration
 * @throws std::invalid_argument if arguments are invalid
 */
    inline ScriptConfig parse_arguments(int argc, char* argv[], bool is_reader) {
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

            case 's':
                try {
                    config.serial_baudrate = parse_serial_baudrate(optarg);
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid serial baudrate: " << optarg << "\n";
                    std::cerr << "Supported: 9600, 19200, 38400, 57600, 115200, 153600, 2000000\n";
                    throw;
                }
                break;

            case 'b':
                try {
                    config.can_baudrate = parse_can_baudrate(optarg);
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid CAN baudrate: " << optarg << "\n";
                    std::cerr <<
                        "Supported: 10000, 20000, 50000, 100000, 125000, 200000, 250000, 400000, 500000, 800000, 1000000\n";
                    throw;
                }
                break;

            case 'f':
                try {
                    config.use_fixed_frames = parse_frame_type(optarg);
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid frame type: " << optarg << "\n";
                    std::cerr << "Use 'fixed' or 'variable'\n";
                    throw;
                }
                break;

            case '?':
                if (optopt == 'd' || optopt == 's' || optopt == 'b' || optopt == 'f') {
                    std::cerr << "Option -" << static_cast<char>(optopt) <<
                        " requires an argument.\n";
                } else {
                    std::cerr << "Unknown option: -" << static_cast<char>(optopt) << "\n";
                }
                throw std::invalid_argument("Invalid command-line option");

            default:
                display_help(argv[0], is_reader);
                throw std::invalid_argument("Invalid command-line arguments");
            }
        }

        return config;
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
 * @brief Initialize USB adapter with configuration
 * @param config Script configuration
 * @return std::unique_ptr<USBAdapter> Initialized adapter (ownership transferred)
 * @throws WaveshareException if initialization fails
 */
    std::unique_ptr<USBAdapter> initialize_adapter(const ScriptConfig& config) {
        // Create and return adapter using factory method
        auto adapter = USBAdapter::create(config.device, config.serial_baudrate);

        // Create and send configuration frame
        auto config_frame = FrameBuilder<ConfigFrame>()
            .with_can_version(config.use_fixed_frames ? CANVersion::STD_FIXED :
            CANVersion::STD_VARIABLE)
            .with_type(Type::CONF_FIXED)
            .with_baud_rate(config.can_baudrate)
            .with_mode(CANMode::NORMAL)
            .with_filter(0x00000000)
            .with_mask(0xFFFFFFFF)
            .build();

        adapter->send_frame(config_frame);  // Throws on error

        return adapter;
    }

} // namespace USBCANBridge

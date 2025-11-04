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
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <getopt.h>

namespace waveshare {

// === Common Utility Functions ===

/**
 * @brief Format CAN data bytes as hex string
 * @param data Pointer to data bytes
 * @param len Number of data bytes
 * @return std::string Formatted hex string (e.g., "DE AD BE EF")
 */
    inline std::string format_can_data(const uint8_t* data, uint8_t len) {
        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::setfill('0');
        for (uint8_t i = 0; i < len; ++i) {
            oss << std::setw(2) << static_cast<int>(data[i]);
            if (i < len - 1) oss << " ";
        }
        return oss.str();
    }

/**
 * @brief Get current timestamp as formatted string
 * @return std::string Timestamp in format "HH:MM:SS.mmm"
 */
    inline std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        auto timer = std::chrono::system_clock::to_time_t(now);
        std::tm bt = *std::localtime(&timer);

        std::ostringstream oss;
        oss << std::put_time(&bt, "%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }

// === Command-Line Argument Parsing ===

/**
 * @brief Writer operation mode enumeration
 */
    enum class WriterMode {
        SINGLE,        // Send one message and exit
        LOOP,          // Send messages in infinite loop
        COUNT,         // Send specific number of messages
        INCREMENT_ID   // Send with incrementing CAN ID
    };

/**
 * @brief Program configuration structure
 */
    struct ScriptConfig {
        std::string device = "/dev/ttyUSB0";
        SerialBaud serial_baudrate = SerialBaud::BAUD_2M;
        CANBaud can_baudrate = CANBaud::BAUD_1M;
        bool use_fixed_frames = false;

        // Bridge-specific configuration
        std::string socketcan_interface = "vcan0";
        CANMode can_mode = CANMode::NORMAL;
        bool auto_retransmit = true;
        std::uint32_t filter_id = 0x00000000;
        std::uint32_t filter_mask = 0x00000000;
        std::uint32_t usb_read_timeout_ms = 100;
        std::uint32_t socketcan_read_timeout_ms = 100;

        // Writer-specific configuration
        WriterMode writer_mode = WriterMode::COUNT;
        std::uint32_t message_count = 10;
        std::uint32_t message_gap_ms = 200;
        std::uint32_t start_id = 0x123;
        std::vector<std::uint8_t> message_data = {0xDE, 0xAD, 0xBE, 0xEF};
        bool increment_id = false;
    };

/**
 * @brief Script type enumeration for help display
 */
    enum class ScriptType {
        READER,
        WRITER,
        BRIDGE
    };

/**
 * @brief Display help message for script usage
 * @param program_name The name of the program (argv[0])
 * @param script_type The type of script (reader, writer, or bridge)
 */
    inline void display_help(const std::string& program_name, ScriptType script_type) {
        std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
        std::cout << "Options:\n";

        if (script_type == ScriptType::BRIDGE) {
            std::cout << "  -i <interface>  SocketCAN interface (default: vcan0)\n";
        }

        std::cout << "  -d <device>     Serial device path (default: /dev/ttyUSB0)\n";
        std::cout << "  -s <baudrate>   Serial baudrate (default: 2000000)\n";
        std::cout <<
            "                  Supported: 9600, 19200, 38400, 57600, 115200, 153600, 2000000\n";
        std::cout << "  -b <baudrate>   CAN bus baudrate (default: 1000000)\n";
        std::cout << "                  Supported: 10000, 20000, 50000, 100000, 125000, 200000,\n";
        std::cout << "                             250000, 400000, 500000, 800000, 1000000\n";

        if (script_type == ScriptType::BRIDGE) {
            std::cout << "  -m <mode>       CAN mode (default: normal)\n";
            std::cout << "                  Supported: normal, loopback, silent, loopback-silent\n";
            std::cout << "  -r <on|off>     Auto-retransmit (default: on)\n";
            std::cout << "                  Supported: on, off, true, false, 1, 0, yes, no\n";
            std::cout <<
                "  -F <id>         CAN filter ID in hex or decimal (default: 0x00000000)\n";
            std::cout <<
                "  -M <mask>       CAN filter mask in hex or decimal (default: 0x00000000)\n";
            std::cout << "  -u <ms>         USB read timeout in milliseconds (default: 100)\n";
            std::cout <<
                "  -t <ms>         SocketCAN read timeout in milliseconds (default: 100)\n";
        }

        if (script_type == ScriptType::WRITER) {
            std::cout << "  -i <id>         CAN message ID in hex or decimal (default: 0x123)\n";
            std::cout << "  -j <data>       CAN data as hex string (default: DEADBEEF)\n";
            std::cout << "                  Example: -j \"DE AD BE EF\" or -j \"DEADBEEF\"\n";
            std::cout <<
                "  -n <count>      Number of messages to send (default: 10, 0 = infinite)\n";
            std::cout <<
                "  -g <ms>         Gap/delay between messages in milliseconds (default: 200)\n";
            std::cout << "  -I              Increment CAN ID for each message sent\n";
            std::cout << "  -l              Loop mode: send messages infinitely (same as -n 0)\n";
        }

        if (script_type != ScriptType::BRIDGE) {
            std::cout <<
                "  -f <type>       Frame type: 'fixed' or 'variable' (default: variable)\n";
        }

        std::cout << "  -h              Display this help message\n";
        std::cout << "\n";

        switch (script_type) {
        case ScriptType::READER:
            std::cout << "Reads CAN frames from the Waveshare USB-CAN adapter and displays them.\n";
            break;
        case ScriptType::WRITER:
            std::cout << "Sends CAN frames to the Waveshare USB-CAN adapter.\n";
            std::cout << "\n";
            std::cout << "Examples:\n";
            std::cout << "  # Send 10 messages with ID 0x123 and default data:\n";
            std::cout << "  " << program_name << " -d /dev/ttyUSB0\n\n";
            std::cout << "  # Send single message with custom ID and data:\n";
            std::cout << "  " << program_name << " -i 0x456 -j \"01 02 03 04\" -n 1\n\n";
            std::cout << "  # Send messages with incrementing ID:\n";
            std::cout << "  " << program_name << " -i 0x100 -I -n 20 -g 100\n\n";
            std::cout << "  # Infinite loop sending messages:\n";
            std::cout << "  " << program_name << " -i 0x200 -j \"CAFEBABE\" -l -g 500\n";
            break;
        case ScriptType::BRIDGE:
            std::cout << "Bridges Waveshare USB-CAN adapter with SocketCAN interface.\n";
            std::cout << "Forwards frames bidirectionally between USB and SocketCAN.\n";
            std::cout << "\n";
            std::cout << "Examples:\n";
            std::cout << "  # Basic bridge with default settings:\n";
            std::cout << "  " << program_name << " -i vcan0 -d /dev/ttyUSB0\n\n";
            std::cout << "  # Bridge with loopback mode and custom filter:\n";
            std::cout << "  " << program_name <<
                " -i vcan0 -d /dev/ttyUSB0 -m loopback -F 0x123 -M 0x7FF\n\n";
            std::cout << "  # High-speed bridge with custom timeouts:\n";
            std::cout << "  " << program_name <<
                " -i can0 -d /dev/ttyUSB0 -s 2000000 -b 1000000 -u 50 -t 50\n";
            break;
        }
    }

// Note: Serial and CAN baudrate parsing functions are available in protocol.hpp:
// - serialbaud_from_int(int baud, bool& use_default) -> SerialBaud
// - canbaud_from_int(int baud, bool& use_default) -> CANBaud
// - canmode_from_string(const std::string& mode_str, bool& use_default) -> CANMode
// - rtx_from_bool(bool value) -> RTX

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
 * @brief Parse boolean from string
 * @param bool_str String representation of boolean ("true"/"false", "1"/"0", "yes"/"no")
 * @return bool Parsed boolean value
 * @throws std::invalid_argument if string is not a valid boolean
 */
    inline bool parse_boolean(const std::string& bool_str) {
        std::string str_lower = bool_str;
        std::transform(str_lower.begin(), str_lower.end(), str_lower.begin(), ::tolower);

        if (str_lower == "true" || str_lower == "1" || str_lower == "yes" || str_lower == "on") {
            return true;
        } else if (str_lower == "false" || str_lower == "0" || str_lower == "no" ||
            str_lower == "off") {
            return false;
        } else {
            throw std::invalid_argument("Invalid boolean value: " + bool_str +
                " (use 'true'/'false', '1'/'0', 'yes'/'no', or 'on'/'off')");
        }
    }

/**
 * @brief Parse hex or decimal integer from string
 * @param value_str String representation of integer (supports 0x prefix for hex)
 * @return std::uint32_t Parsed integer value
 * @throws std::invalid_argument if string is not a valid integer
 */
    inline std::uint32_t parse_uint32(const std::string& value_str) {
        std::uint32_t value;
        std::size_t pos = 0;

        // Check if hex format (0x prefix)
        if (value_str.size() >= 2 && value_str[0] == '0' &&
            (value_str[1] == 'x' || value_str[1] == 'X')) {
            value = std::stoul(value_str, &pos, 16);
        } else {
            value = std::stoul(value_str, &pos, 10);
        }

        if (pos != value_str.size()) {
            throw std::invalid_argument("Invalid integer format: " + value_str);
        }

        return value;
    }

/**
 * @brief Parse hex string to binary data
 * @param hex_str Hex string (e.g., "DEADBEEF" or "DE AD BE EF")
 * @return std::vector<std::uint8_t> Binary data
 * @throws std::invalid_argument if string contains invalid hex characters
 */
    inline std::vector<std::uint8_t> parse_hex_data(const std::string& hex_str) {
        std::vector<std::uint8_t> data;
        std::string clean_str;

        // Remove spaces and other non-hex characters
        for (char c : hex_str) {
            if (std::isxdigit(c)) {
                clean_str += c;
            } else if (c != ' ' && c != ':' && c != '-') {
                throw std::invalid_argument("Invalid hex character in data string");
            }
        }

        // Must have even number of hex digits
        if (clean_str.size() % 2 != 0) {
            throw std::invalid_argument("Hex data string must have even number of digits");
        }

        // Parse pairs of hex digits
        for (size_t i = 0; i < clean_str.size(); i += 2) {
            std::string byte_str = clean_str.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
            data.push_back(byte);
        }

        if (data.size() > 8) {
            throw std::invalid_argument("CAN data cannot exceed 8 bytes");
        }

        return data;
    }

/**
 * @brief Parse command-line arguments
 * @param argc Argument count
 * @param argv Argument values
 * @param script_type The type of script (reader, writer, or bridge)
 * @return ScriptConfig Parsed configuration
 * @throws std::invalid_argument if arguments are invalid
 */
    inline ScriptConfig parse_arguments(int argc, char* argv[], ScriptType script_type) {
        ScriptConfig config;
        int opt;
        bool baud_not_found = false;

        const char* optstring;
        if (script_type == ScriptType::BRIDGE) {
            optstring = "hi:d:s:b:m:r:F:M:u:t:";
        } else if (script_type == ScriptType::WRITER) {
            optstring = "hd:s:b:f:i:j:n:g:Il";
        } else { // READER
            optstring = "hd:s:b:f:";
        }

        while ((opt = getopt(argc, argv, optstring)) != -1) {
            switch (opt) {
            case 'h':
                display_help(argv[0], script_type);
                std::exit(0);

            case 'i':
                if (script_type == ScriptType::BRIDGE) {
                    // SocketCAN interface (bridge only)
                    config.socketcan_interface = optarg;
                } else if (script_type == ScriptType::WRITER) {
                    // CAN message ID (writer only)
                    try {
                        config.start_id = parse_uint32(optarg);
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "Invalid CAN ID: " << optarg << "\n";
                        std::cerr << "Use decimal or hex (0x...) format\n";
                        throw;
                    }
                }
                break;

            case 'j':  // CAN data (writer only)
                if (script_type == ScriptType::WRITER) {
                    try {
                        config.message_data = parse_hex_data(optarg);
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "Invalid CAN data: " << optarg << "\n";
                        std::cerr << "Use hex format (e.g., \"DEADBEEF\" or \"DE AD BE EF\")\n";
                        throw;
                    }
                }
                break;

            case 'n':  // Message count (writer only)
                if (script_type == ScriptType::WRITER) {
                    try {
                        config.message_count = parse_uint32(optarg);
                        if (config.message_count == 0) {
                            config.writer_mode = WriterMode::LOOP;
                        } else {
                            config.writer_mode = WriterMode::COUNT;
                        }
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "Invalid message count: " << optarg << "\n";
                        throw;
                    }
                }
                break;

            case 'g':  // Message gap/delay (writer only)
                if (script_type == ScriptType::WRITER) {
                    try {
                        config.message_gap_ms = parse_uint32(optarg);
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "Invalid message gap: " << optarg << "\n";
                        throw;
                    }
                }
                break;

            case 'I':  // Increment ID (writer only)
                if (script_type == ScriptType::WRITER) {
                    config.increment_id = true;
                }
                break;

            case 'l':  // Loop mode (writer only)
                if (script_type == ScriptType::WRITER) {
                    config.writer_mode = WriterMode::LOOP;
                    config.message_count = 0;
                }
                break;

            case 'd':
                config.device = optarg;
                break;

            case 's':
                try {
                    config.serial_baudrate = serialbaud_from_int(std::stoi(optarg), baud_not_found);
                    if (baud_not_found) {
                        throw std::invalid_argument("Unsupported serial baudrate: " +
                            std::string(optarg));
                    }
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid serial baudrate: " << optarg << "\n";
                    std::cerr << "Supported: 9600, 19200, 38400, 57600, 115200, 153600, 2000000\n";
                    throw;
                }
                break;

            case 'b':
                try {
                    config.can_baudrate = canbaud_from_int(std::stoi(optarg), baud_not_found);
                    if (baud_not_found) {
                        throw std::invalid_argument("Unsupported CAN baudrate: " +
                            std::string(optarg));
                    }
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid CAN baudrate: " << optarg << "\n";
                    std::cerr <<
                        "Supported: 10000, 20000, 50000, 100000, 125000, 200000, 250000, 400000, 500000, 800000, 1000000\n";
                    throw;
                }
                break;

            case 'm':  // CAN mode (bridge only)
                if (script_type == ScriptType::BRIDGE) {
                    try {
                        bool mode_not_found = false;
                        config.can_mode = canmode_from_string(optarg, mode_not_found);
                        if (mode_not_found) {
                            throw std::invalid_argument("Unsupported CAN mode: " +
                                std::string(optarg));
                        }
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "Invalid CAN mode: " << optarg << "\n";
                        std::cerr << "Supported: normal, loopback, silent, loopback_silent\n";
                        throw;
                    }
                }
                break;

            case 'r':  // Auto-retransmit (bridge only)
                if (script_type == ScriptType::BRIDGE) {
                    try {
                        config.auto_retransmit = parse_boolean(optarg);
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "Invalid auto-retransmit value: " << optarg << "\n";
                        std::cerr << "Supported: on, off, true, false, 1, 0, yes, no\n";
                        throw;
                    }
                }
                break;

            case 'F':  // Filter ID (bridge only)
                if (script_type == ScriptType::BRIDGE) {
                    try {
                        config.filter_id = parse_uint32(optarg);
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "Invalid filter ID: " << optarg << "\n";
                        std::cerr << "Use decimal or hex (0x...) format\n";
                        throw;
                    }
                }
                break;

            case 'M':  // Filter mask (bridge only)
                if (script_type == ScriptType::BRIDGE) {
                    try {
                        config.filter_mask = parse_uint32(optarg);
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "Invalid filter mask: " << optarg << "\n";
                        std::cerr << "Use decimal or hex (0x...) format\n";
                        throw;
                    }
                }
                break;

            case 'u':  // USB timeout (bridge only)
                if (script_type == ScriptType::BRIDGE) {
                    try {
                        config.usb_read_timeout_ms = parse_uint32(optarg);
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "Invalid USB timeout: " << optarg << "\n";
                        std::cerr << "Use positive integer (milliseconds)\n";
                        throw;
                    }
                }
                break;

            case 't':  // SocketCAN timeout (bridge only)
                if (script_type == ScriptType::BRIDGE) {
                    try {
                        config.socketcan_read_timeout_ms = parse_uint32(optarg);
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "Invalid SocketCAN timeout: " << optarg << "\n";
                        std::cerr << "Use positive integer (milliseconds)\n";
                        throw;
                    }
                }
                break;

            case 'f':  // Frame type (reader/writer only)
                if (script_type != ScriptType::BRIDGE) {
                    try {
                        config.use_fixed_frames = parse_frame_type(optarg);
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "Invalid frame type: " << optarg << "\n";
                        std::cerr << "Use 'fixed' or 'variable'\n";
                        throw;
                    }
                }
                break;

            case '?':
                if (optopt == 'd' || optopt == 's' || optopt == 'b' || optopt == 'f' ||
                    optopt == 'i' || optopt == 'm' || optopt == 'r' || optopt == 'F' ||
                    optopt == 'M' || optopt == 'u' || optopt == 't' || optopt == 'j' ||
                    optopt == 'n' || optopt == 'g') {
                    std::cerr << "Option -" << static_cast<char>(optopt) <<
                        " requires an argument.\n";
                } else {
                    std::cerr << "Unknown option: -" << static_cast<char>(optopt) << "\n";
                }
                throw std::invalid_argument("Invalid command-line option");

            default:
                display_help(argv[0], script_type);
                throw std::invalid_argument("Invalid command-line arguments");
            }
        }

        return config;
    }

// === Adapter Configuration ===

    /**
     * @brief Initialize USB adapter with configuration
     * @param config Script configuration
     * @param rtx_mode RTX mode (default: AUTO)
     * @return std::unique_ptr<USBAdapter> Initialized adapter (ownership transferred)
     * @throws WaveshareException if initialization fails
     */
    std::unique_ptr<USBAdapter> initialize_adapter(const ScriptConfig& config,
        RTX rtx_mode = RTX::AUTO) {
        // Create and return adapter using factory method
        auto adapter = USBAdapter::create(config.device, config.serial_baudrate);

        // Create and send configuration frame
        // NOTE: Config frames always use STD_FIXED for CAN_VERS, regardless of data frame type
        auto config_frame = FrameBuilder<ConfigFrame>()
            .with_can_version(CANVersion::STD_FIXED)  // Always use STD_FIXED for config
            .with_baud_rate(config.can_baudrate)
            .with_mode(CANMode::NORMAL)
            .with_rtx(rtx_mode)        // Use specified RTX mode
            .with_filter(0x00000000)
            .with_mask(0x00000000)
            .build();

        std::cout << "Sending configuration frame to adapter:" << config_frame.to_string() <<
            std::endl;
        adapter->send_frame(config_frame);  // Throws on error

        return adapter;
    }
}      // namespace USBCANBridge

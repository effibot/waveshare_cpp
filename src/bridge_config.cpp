/**
 * @file bridge_config.cpp
 * @brief Configuration structure implementation
 * @version 1.0
 * @date 2025-10-14
 */

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <unistd.h>

#include "../include/pattern/bridge_config.hpp"
#include "../include/exception/waveshare_exception.hpp"

namespace waveshare {

    // === Configuration Validation ===

    void BridgeConfig::validate() const {
        // Validate interface name
        if (socketcan_interface.empty()) {
            throw std::invalid_argument("SocketCAN interface name cannot be empty");
        }

        // Validate USB device path
        if (usb_device_path.empty()) {
            throw std::invalid_argument("USB device path cannot be empty");
        }

        // NOTE: Hardware existence checks (access(), socket creation) are NOT done here.
        // Those belong in factory methods (create()) or during actual connection.
        // validate() should only check LOGICAL validity of configuration parameters.

        // Validate timeouts
        if (usb_read_timeout_ms == 0) {
            throw std::invalid_argument("USB read timeout must be > 0");
        }
        if (socketcan_read_timeout_ms == 0) {
            throw std::invalid_argument("SocketCAN read timeout must be > 0");
        }
        if (usb_read_timeout_ms > 60000) {
            throw std::invalid_argument("USB read timeout too large (max 60000ms)");
        }
        if (socketcan_read_timeout_ms > 60000) {
            throw std::invalid_argument("SocketCAN read timeout too large (max 60000ms)");
        }

        // Validate filter/mask based on standard vs extended
        // Note: We can't determine if using standard or extended here,
        // so we just ensure they fit in 29 bits (extended max)
        if (filter_id > 0x1FFFFFFF) {
            throw std::invalid_argument("Filter ID exceeds 29-bit maximum");
        }
        if (filter_mask > 0x1FFFFFFF) {
            throw std::invalid_argument("Filter mask exceeds 29-bit maximum");
        }
    }

    // === Factory Methods ===

    BridgeConfig BridgeConfig::create_default() {
        BridgeConfig config;
        config.socketcan_interface = "vcan0";
        config.usb_device_path = "/dev/ttyUSB0";
        config.serial_baud_rate = SerialBaud::BAUD_2M;
        config.can_baud_rate = CANBaud::BAUD_1M;
        config.can_mode = CANMode::NORMAL;
        config.auto_retransmit = true;
        config.filter_id = 0;
        config.filter_mask = 0;
        config.usb_read_timeout_ms = 100;
        config.socketcan_read_timeout_ms = 100;
        return config;
    }

    // === Environment Variable Helpers ===

    std::string BridgeConfig::get_env(const std::string& name, const std::string& default_val) {
        const char* val = std::getenv(name.c_str());
        return val ? std::string(val) : default_val;
    }

    // === .env File Parsing ===

    std::map<std::string, std::string> BridgeConfig::parse_env_file(const std::string& filepath) {
        std::map<std::string, std::string> result;
        std::ifstream file(filepath);

        if (!file.is_open()) {
            throw std::runtime_error("Cannot open .env file: " + filepath);
        }

        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }

            // Find '=' separator
            size_t eq_pos = line.find('=');
            if (eq_pos == std::string::npos) {
                continue;  // Skip malformed lines
            }

            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);

            // Remove quotes if present
            if (value.size() >= 2 &&
                ((value.front() == '"' && value.back() == '"') ||
                (value.front() == '\'' && value.back() == '\''))) {
                value = value.substr(1, value.size() - 2);
            }

            result[key] = value;
        }

        return result;
    }

    // === Configuration Application ===

    void BridgeConfig::apply_config_map(BridgeConfig& config,
        const std::map<std::string, std::string>& vars) {
        // Helper to get value with key
        auto get_val = [&vars](const std::string& key) -> std::optional<std::string> {
                auto it = vars.find(key);
                if (it != vars.end()) {
                    return it->second;
                }
                return std::nullopt;
            };

        // Apply string configs
        if (auto val = get_val("WAVESHARE_SOCKETCAN_INTERFACE")) {
            config.socketcan_interface = *val;
        }
        if (auto val = get_val("WAVESHARE_USB_DEVICE")) {
            config.usb_device_path = *val;
        }

        // Apply serial baud rate
        if (auto val = get_val("WAVESHARE_SERIAL_BAUD")) {
            int baud = std::stoi(*val);
            switch (baud) {
            case 9600:    config.serial_baud_rate = SerialBaud::BAUD_9600; break;
            case 19200:   config.serial_baud_rate = SerialBaud::BAUD_19200; break;
            case 38400:   config.serial_baud_rate = SerialBaud::BAUD_38400; break;
            case 57600:   config.serial_baud_rate = SerialBaud::BAUD_57600; break;
            case 115200:  config.serial_baud_rate = SerialBaud::BAUD_115200; break;
            case 153600:  config.serial_baud_rate = SerialBaud::BAUD_153600; break;
            case 2000000: config.serial_baud_rate = SerialBaud::BAUD_2M; break;
            default:
                throw std::invalid_argument("Invalid serial baud rate: " + *val);
            }
        }

        // Apply CAN baud rate
        if (auto val = get_val("WAVESHARE_CAN_BAUD")) {
            int baud = std::stoi(*val);
            switch (baud) {
            case 5000:    config.can_baud_rate = CANBaud::BAUD_5K; break;
            case 10000:   config.can_baud_rate = CANBaud::BAUD_10K; break;
            case 20000:   config.can_baud_rate = CANBaud::BAUD_20K; break;
            case 50000:   config.can_baud_rate = CANBaud::BAUD_50K; break;
            case 100000:  config.can_baud_rate = CANBaud::BAUD_100K; break;
            case 125000:  config.can_baud_rate = CANBaud::BAUD_125K; break;
            case 200000:  config.can_baud_rate = CANBaud::BAUD_200K; break;
            case 250000:  config.can_baud_rate = CANBaud::BAUD_250K; break;
            case 400000:  config.can_baud_rate = CANBaud::BAUD_400K; break;
            case 500000:  config.can_baud_rate = CANBaud::BAUD_500K; break;
            case 800000:  config.can_baud_rate = CANBaud::BAUD_800K; break;
            case 1000000: config.can_baud_rate = CANBaud::BAUD_1M; break;
            default:
                throw std::invalid_argument("Invalid CAN baud rate: " + *val);
            }
        }

        // Apply CAN mode
        if (auto val = get_val("WAVESHARE_CAN_MODE")) {
            std::string mode = *val;
            // Convert to lowercase for case-insensitive comparison
            std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);

            if (mode == "normal") {
                config.can_mode = CANMode::NORMAL;
            } else if (mode == "loopback") {
                config.can_mode = CANMode::LOOPBACK;
            } else if (mode == "silent") {
                config.can_mode = CANMode::SILENT;
            } else if (mode == "loopback-silent" || mode == "loopback_silent" ||
                mode == "silent-loopback" || mode == "silent_loopback") {
                config.can_mode = CANMode::LOOPBACK_SILENT;
            } else {
                throw std::invalid_argument("Invalid CAN mode: " + *val);
            }
        }

        // Apply auto retransmit
        if (auto val = get_val("WAVESHARE_AUTO_RETRANSMIT")) {
            std::string rtx = *val;
            std::transform(rtx.begin(), rtx.end(), rtx.begin(), ::tolower);
            config.auto_retransmit = (rtx == "true" || rtx == "1" || rtx == "yes");
        }

        // Apply filter/mask
        if (auto val = get_val("WAVESHARE_FILTER_ID")) {
            config.filter_id = std::stoul(*val, nullptr, 0);  // Support hex (0x...)
        }
        if (auto val = get_val("WAVESHARE_FILTER_MASK")) {
            config.filter_mask = std::stoul(*val, nullptr, 0);
        }

        // Apply timeouts
        if (auto val = get_val("WAVESHARE_USB_READ_TIMEOUT")) {
            config.usb_read_timeout_ms = std::stoul(*val);
        }
        if (auto val = get_val("WAVESHARE_SOCKETCAN_READ_TIMEOUT")) {
            config.socketcan_read_timeout_ms = std::stoul(*val);
        }
    }

    // === Load Methods ===

    BridgeConfig BridgeConfig::from_env(bool use_defaults) {
        BridgeConfig config = use_defaults ? create_default() : BridgeConfig{};

        // Build map from environment variables
        std::map<std::string, std::string> env_vars;

        const char* val;
        if ((val = std::getenv("WAVESHARE_SOCKETCAN_INTERFACE"))) {
            env_vars["WAVESHARE_SOCKETCAN_INTERFACE"] = val;
        }
        if ((val = std::getenv("WAVESHARE_USB_DEVICE"))) {
            env_vars["WAVESHARE_USB_DEVICE"] = val;
        }
        if ((val = std::getenv("WAVESHARE_SERIAL_BAUD"))) {
            env_vars["WAVESHARE_SERIAL_BAUD"] = val;
        }
        if ((val = std::getenv("WAVESHARE_CAN_BAUD"))) {
            env_vars["WAVESHARE_CAN_BAUD"] = val;
        }
        if ((val = std::getenv("WAVESHARE_CAN_MODE"))) {
            env_vars["WAVESHARE_CAN_MODE"] = val;
        }
        if ((val = std::getenv("WAVESHARE_AUTO_RETRANSMIT"))) {
            env_vars["WAVESHARE_AUTO_RETRANSMIT"] = val;
        }
        if ((val = std::getenv("WAVESHARE_FILTER_ID"))) {
            env_vars["WAVESHARE_FILTER_ID"] = val;
        }
        if ((val = std::getenv("WAVESHARE_FILTER_MASK"))) {
            env_vars["WAVESHARE_FILTER_MASK"] = val;
        }
        if ((val = std::getenv("WAVESHARE_USB_READ_TIMEOUT"))) {
            env_vars["WAVESHARE_USB_READ_TIMEOUT"] = val;
        }
        if ((val = std::getenv("WAVESHARE_SOCKETCAN_READ_TIMEOUT"))) {
            env_vars["WAVESHARE_SOCKETCAN_READ_TIMEOUT"] = val;
        }

        apply_config_map(config, env_vars);
        return config;
    }

    BridgeConfig BridgeConfig::from_file(const std::string& filepath, bool use_defaults) {
        BridgeConfig config = use_defaults ? create_default() : BridgeConfig{};

        auto file_vars = parse_env_file(filepath);
        apply_config_map(config, file_vars);

        return config;
    }

    BridgeConfig BridgeConfig::load(const std::optional<std::string>& env_file_path) {
        // Start with defaults
        BridgeConfig config = create_default();

        // Apply .env file if provided
        if (env_file_path.has_value()) {
            auto file_vars = parse_env_file(*env_file_path);
            apply_config_map(config, file_vars);
        }

        // Apply environment variables (highest priority)
        // Build map from environment variables that are actually set
        std::map<std::string, std::string> env_vars;

        const char* val;
        if ((val = std::getenv("WAVESHARE_SOCKETCAN_INTERFACE"))) {
            env_vars["WAVESHARE_SOCKETCAN_INTERFACE"] = val;
        }
        if ((val = std::getenv("WAVESHARE_USB_DEVICE"))) {
            env_vars["WAVESHARE_USB_DEVICE"] = val;
        }
        if ((val = std::getenv("WAVESHARE_SERIAL_BAUD"))) {
            env_vars["WAVESHARE_SERIAL_BAUD"] = val;
        }
        if ((val = std::getenv("WAVESHARE_CAN_BAUD"))) {
            env_vars["WAVESHARE_CAN_BAUD"] = val;
        }
        if ((val = std::getenv("WAVESHARE_CAN_MODE"))) {
            env_vars["WAVESHARE_CAN_MODE"] = val;
        }
        if ((val = std::getenv("WAVESHARE_AUTO_RETRANSMIT"))) {
            env_vars["WAVESHARE_AUTO_RETRANSMIT"] = val;
        }
        if ((val = std::getenv("WAVESHARE_FILTER_ID"))) {
            env_vars["WAVESHARE_FILTER_ID"] = val;
        }
        if ((val = std::getenv("WAVESHARE_FILTER_MASK"))) {
            env_vars["WAVESHARE_FILTER_MASK"] = val;
        }
        if ((val = std::getenv("WAVESHARE_USB_READ_TIMEOUT"))) {
            env_vars["WAVESHARE_USB_READ_TIMEOUT"] = val;
        }
        if ((val = std::getenv("WAVESHARE_SOCKETCAN_READ_TIMEOUT"))) {
            env_vars["WAVESHARE_SOCKETCAN_READ_TIMEOUT"] = val;
        }

        // Apply environment variables over file config
        if (!env_vars.empty()) {
            apply_config_map(config, env_vars);
        }

        return config;
    }

} // namespace waveshare

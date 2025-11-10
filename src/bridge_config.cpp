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

#include <nlohmann/json.hpp>

#include "../include/pattern/bridge_config.hpp"
#include "../include/exception/waveshare_exception.hpp"

using json = nlohmann::json;

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

    // === JSON File Parsing ===

    BridgeConfig BridgeConfig::from_json(const json& j) {
        BridgeConfig config = create_default();

        // Convert JSON to string map to reuse apply_config_map logic
        std::map<std::string, std::string> config_map;

        if (j.contains("bridge_config")) {
            const auto& bc = j["bridge_config"];

            // Convert JSON fields to WAVESHARE_* format to reuse existing parsing logic
            if (bc.contains("socketcan_interface")) {
                config_map["WAVESHARE_SOCKETCAN_INTERFACE"] = bc["socketcan_interface"].get<std::string>();
            }
            if (bc.contains("usb_device_path")) {
                config_map["WAVESHARE_USB_DEVICE"] = bc["usb_device_path"].get<std::string>();
            }
            if (bc.contains("serial_baud_rate")) {
                config_map["WAVESHARE_SERIAL_BAUD"] =
                    std::to_string(bc["serial_baud_rate"].get<int>());
            }
            if (bc.contains("can_baud_rate")) {
                config_map["WAVESHARE_CAN_BAUD"] = std::to_string(bc["can_baud_rate"].get<int>());
            }
            if (bc.contains("can_mode")) {
                config_map["WAVESHARE_CAN_MODE"] = bc["can_mode"].get<std::string>();
            }
            if (bc.contains("auto_retransmit")) {
                config_map["WAVESHARE_AUTO_RETRANSMIT"] =
                    bc["auto_retransmit"].get<bool>() ? "true" : "false";
            }
            if (bc.contains("filter_id")) {
                config_map["WAVESHARE_FILTER_ID"] = std::to_string(bc["filter_id"].get<uint32_t>());
            }
            if (bc.contains("filter_mask")) {
                config_map["WAVESHARE_FILTER_MASK"] =
                    std::to_string(bc["filter_mask"].get<uint32_t>());
            }
            if (bc.contains("usb_read_timeout_ms")) {
                config_map["WAVESHARE_USB_READ_TIMEOUT"] =
                    std::to_string(bc["usb_read_timeout_ms"].get<uint32_t>());
            }
            if (bc.contains("socketcan_read_timeout_ms")) {
                config_map["WAVESHARE_SOCKETCAN_READ_TIMEOUT"] =
                    std::to_string(bc["socketcan_read_timeout_ms"].get<uint32_t>());
            }
        }

        // Reuse existing parsing logic
        apply_config_map(config, config_map);

        return config;
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
            bool use_default = false;
            config.serial_baud_rate = serialbaud_from_int(baud, use_default);
            if (use_default) {
                throw std::invalid_argument("Invalid serial baud rate: " + *val);
            }
        }

        // Apply CAN baud rate
        if (auto val = get_val("WAVESHARE_CAN_BAUD")) {
            int baud = std::stoi(*val);
            bool use_default = false;
            config.can_baud_rate = canbaud_from_int(baud, use_default);
            if (use_default) {
                throw std::invalid_argument("Invalid CAN baud rate: " + *val);
            }
        }

        // Apply CAN mode
        if (auto val = get_val("WAVESHARE_CAN_MODE")) {
            std::string mode = *val;
            // Convert to lowercase for case-insensitive comparison
            std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
            // Replace dash with underscore for consistency (loopback-silent -> loopback_silent)
            std::replace(mode.begin(), mode.end(), '-', '_');

            bool use_default = false;
            config.can_mode = canmode_from_string(mode, use_default);
            if (use_default) {
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

    BridgeConfig BridgeConfig::from_file(const std::string& filepath, bool use_defaults) {
        BridgeConfig config = use_defaults ? create_default() : BridgeConfig{};

        // Parse as JSON file
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open JSON config file: " + filepath);
        }

        try {
            json j;
            file >> j;
            config = from_json(j);
        } catch (const json::exception& e) {
            throw std::runtime_error("JSON parse error in " + filepath + ": " + e.what());
        }

        return config;
    }

    BridgeConfig BridgeConfig::load(const std::optional<std::string>& config_file_path) {
        // Start with defaults
        BridgeConfig config = create_default();

        // Apply JSON config file if provided
        if (config_file_path.has_value()) {
            std::ifstream file(*config_file_path);
            if (file.is_open()) {
                try {
                    json j;
                    file >> j;
                    config = from_json(j);
                } catch (const json::exception& e) {
                    throw std::runtime_error("JSON parse error in " + *config_file_path + ": " +
                        e.what());
                }
            }
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

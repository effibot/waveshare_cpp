/**
 * @file bridge_config.hpp
 * @brief Configuration structure for SocketCAN bridge
 * @version 1.0
 * @date 2025-10-14
 *
 * Supports multiple configuration sources:
 * 1. Environment variables (WAVESHARE_*)
 * 2. .env file parsing
 * 3. Programmatic defaults
 * 4. Direct construction
 *
 * Priority: Direct args > Env vars > .env file > Defaults
 *
 * @copyright Copyright (c) 2025
 */

#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include <map>

#include "../enums/protocol.hpp"

namespace waveshare {

    /**
     * @brief Configuration for SocketCAN-Waveshare bridge
     *
     * Environment Variables:
     *
     * - WAVESHARE_SOCKETCAN_INTERFACE: SocketCAN interface name (default: "vcan0")
     *
     * - WAVESHARE_USB_DEVICE: USB device path (default: "/dev/ttyUSB0")
     *
     * - WAVESHARE_SERIAL_BAUD: Serial baud rate in bps (default: 2000000)
     *
     * - WAVESHARE_CAN_BAUD: CAN bus baud rate enum value (default: 1000000)
     *
     * - WAVESHARE_CAN_MODE: CAN mode (normal/loopback/silent/silent-loopback, default: normal)
     *
     * - WAVESHARE_AUTO_RETRANSMIT: Auto retransmit (true/false, default: true)
     *
     * - WAVESHARE_FILTER_ID: CAN filter ID (default: 0)
     *
     * - WAVESHARE_FILTER_MASK: CAN filter mask (default: 0)
     *
     * - WAVESHARE_USB_READ_TIMEOUT: USB read timeout in ms (default: 100)
     *
     * - WAVESHARE_SOCKETCAN_READ_TIMEOUT: SocketCAN read timeout in ms (default: 100)
     */
    struct BridgeConfig {
        // === Network Configuration ===
        std::string socketcan_interface = "vcan0";
        std::string usb_device_path = "/dev/ttyUSB0";

        // === Communication Parameters ===
        SerialBaud serial_baud_rate = SerialBaud::BAUD_2M;
        CANBaud can_baud_rate = CANBaud::BAUD_1M;
        CANMode can_mode = CANMode::NORMAL;
        bool auto_retransmit = true;

        // === Filtering ===
        std::uint32_t filter_id = 0;
        std::uint32_t filter_mask = 0;

        // === Timeouts (milliseconds) ===
        std::uint32_t usb_read_timeout_ms = 100;
        std::uint32_t socketcan_read_timeout_ms = 100;

        /**
         * @brief Validate configuration
         * @throws std::invalid_argument if config is invalid
         * @throws DeviceException if USB device doesn't exist
         */
        void validate() const;

        /**
         * @brief Create default configuration
         * @return BridgeConfig with sensible defaults
         */
        static BridgeConfig create_default();

        /**
         * @brief Load configuration from environment variables
         * @param use_defaults If true, use defaults for unset vars; if false, only use env vars
         * @return BridgeConfig loaded from environment
         */
        static BridgeConfig from_env(bool use_defaults = true);

        /**
         * @brief Load configuration from .env file
         * @param filepath Path to .env file
         * @param use_defaults If true, use defaults for unset vars
         * @return BridgeConfig loaded from file
         * @throws std::runtime_error if file cannot be read
         */
        static BridgeConfig from_file(const std::string& filepath, bool use_defaults = true);

        /**
         * @brief Load configuration with priority: env vars > .env file > defaults
         * @param env_file_path Optional path to .env file
         * @return BridgeConfig with merged settings
         */
        static BridgeConfig load(const std::optional<std::string>& env_file_path = std::nullopt);

        private:
            /**
             * @brief Parse .env file into key-value map
             * @param filepath Path to .env file
             * @return Map of keys to values
             */
            static std::map<std::string, std::string> parse_env_file(const std::string& filepath);

            /**
             * @brief Apply configuration from key-value map
             * @param config Configuration to update
             * @param vars Key-value pairs
             */
            static void apply_config_map(BridgeConfig& config,
                const std::map<std::string, std::string>& vars);

            /**
             * @brief Get environment variable with optional default
             * @param name Variable name
             * @param default_val Default value if not set
             * @return Variable value or default
             */
            static std::string get_env(const std::string& name,
                const std::string& default_val = "");
    };

} // namespace waveshare

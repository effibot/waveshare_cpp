/**
 * @file test_utils.hpp
 * @brief Test utility functions for creating mocked components
 * @version 1.0
 * @date 2025-10-14
 *
 * Provides helper functions to create SocketCANBridge and USBAdapter instances
 * with mock dependencies injected, eliminating hardware requirements in tests.
 */

#pragma once

#include "../include/pattern/socketcan_bridge.hpp"
#include "../include/pattern/usb_adapter.hpp"
#include "../include/pattern/bridge_config.hpp"
#include "mocks/mock_can_socket.hpp"
#include "mocks/mock_serial_port.hpp"
#include <memory>

namespace waveshare {
    namespace test {

        /**
         * @brief Create USBAdapter with mock serial port
         * @param config Bridge configuration (uses usb_device_path, serial_baud_rate)
         * @return Unique pointer to USBAdapter with MockSerialPort injected
         *
         * Example:
         * @code
         * BridgeConfig config = BridgeConfig::create_default();
         * auto adapter = create_usb_adapter_with_mock(config);
         * // Use adapter for testing without /dev/ttyUSB0
         * @endcode
         */
        inline std::unique_ptr<USBAdapter> create_usb_adapter_with_mock(
            const BridgeConfig& config) {
            // Create mock serial port (baud rate not needed for mock)
            auto mock_serial = std::make_unique<MockSerialPort>(config.usb_device_path);

            // Inject mock into USBAdapter via DI constructor
            return std::make_unique<USBAdapter>(
                std::move(mock_serial),
                config.usb_device_path,
                config.serial_baud_rate
            );
        }

        /**
         * @brief Create SocketCANBridge with mock CAN socket and mock USB adapter
         * @param config Bridge configuration
         * @return Unique pointer to SocketCANBridge with all mocks injected
         *
         * Example:
         * @code
         * BridgeConfig config = BridgeConfig::create_default();
         * auto bridge = create_bridge_with_mocks(config);
         * bridge->start();  // No hardware required!
         * @endcode
         */
        inline std::unique_ptr<SocketCANBridge> create_bridge_with_mocks(
            const BridgeConfig& config) {
            // Create mock CAN socket
            auto mock_can = std::make_unique<MockCANSocket>(
                config.socketcan_interface,
                config.socketcan_read_timeout_ms
            );

            // Create mock USB adapter
            auto mock_usb = create_usb_adapter_with_mock(config);

            // Inject mocks into bridge via DI constructor
            return std::make_unique<SocketCANBridge>(
                config,
                std::move(mock_can),
                std::move(mock_usb)
            );
        }

    } // namespace test
} // namespace waveshare

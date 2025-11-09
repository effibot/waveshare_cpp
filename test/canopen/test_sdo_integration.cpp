/**
 * @file test_sdo_integration.cpp
 * @brief Integration tests for SDO communication with mock motor driver
 * @author effibot (andrea.efficace1@gmail.com)
 * @date 2025-11-06
 *
 * This test requires:
 * 1. vcan_test interface up: sudo ip link add dev vcan_test type vcan && sudo ip link set vcan_test up
 * 2. MockMotorResponder automatically started by test fixture
 * 3. Motor config file: config/motor_config.json
 *
 * Note: Uses vcan_test instead of vcan0 to avoid conflicts with running bridges/motors.
 *
 * Usage:
 *   ./build/test/test_sdo_integration
 *
 * This test will:
 * - Read Statusword (0x6041) to check motor state
 * - Read Error Register (0x1001) to detect faults
 * - Read Mode of Operation Display (0x6061)
 * - Read Position and Velocity actual values
 * - Write Mode of Operation (0x6060)
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "canopen/object_dictionary.hpp"
#include "canopen/sdo_client.hpp"
#include "canopen/cia402_constants.hpp"
#include "test_utils_canopen.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

using namespace canopen;
using namespace canopen::cia402;
using namespace test_utils;
using Catch::Matchers::ContainsSubstring;

// ==============================================================================
// HELPER FUNCTIONS
// ==============================================================================

/**
 * @brief Decode CIA402 statusword into human-readable state
 */
std::string decode_statusword_string(uint16_t statusword) {
    State state = decode_statusword(statusword);
    return state_to_string(state);
}/**
  * @brief Check if vcan_test interface is available
  */
bool is_vcan_test_available() {
    return std::filesystem::exists("/sys/class/net/vcan_test");
}

/**
 * @brief Check if motor config file exists
 */
bool is_motor_config_available() {
    // Try multiple possible paths
    std::vector<std::string> possible_paths = {
        "../config/motor_config.json",
        "../../config/motor_config.json",
        "../../../src/ros2_waveshare/config/motor_config.json",
        "/home/ros/ws/ros2_canbus_control/src/ros2_waveshare/config/motor_config.json",
        "/home/ros/ws/src/ros2_waveshare/config/motor_config.json"
    };

    for (const auto& path : possible_paths) {
        if (std::filesystem::exists(path)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Get the motor config file path
 */
std::string get_motor_config_path() {
    std::vector<std::string> possible_paths = {
        "../config/motor_config.json",
        "../../config/motor_config.json",
        "../../../src/ros2_waveshare/config/motor_config.json",
        "/home/ros/ws/ros2_canbus_control/src/ros2_waveshare/config/motor_config.json",
        "/home/ros/ws/src/ros2_waveshare/config/motor_config.json"
    };

    for (const auto& path : possible_paths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    throw std::runtime_error("motor_config.json not found");
}

// ==============================================================================
// INTEGRATION TESTS
// ==============================================================================

TEST_CASE_METHOD(CANopenIntegrationFixture, "SDO Integration: Environment Check",
    "[integration][sdo]") {
    SECTION("vcan_test interface availability") {
        if (!is_vcan_test_available()) {
            WARN(
                "vcan_test interface not found. Run: sudo ip link add dev vcan0 type vcan && sudo ip link set vcan0 up");
            SKIP("vcan0 not available - skipping integration tests");
        }
        REQUIRE(is_vcan_test_available());
        INFO("✓ vcan_test interface is available");
    }

    SECTION("motor_config.json availability") {
        if (!is_motor_config_available()) {
            WARN("motor_config.json not found - integration tests will be skipped");
        }
        // Don't fail if config not found, just warn
    }
}

TEST_CASE_METHOD(CANopenIntegrationFixture, "SDO Integration: Read Statusword",
    "[integration][sdo][statusword]") {
    if (!is_vcan_available() || !is_motor_config_available()) {
        SKIP("Prerequisites not met (vcan0 or motor_config.json missing)");
    }

    INFO("=== Reading Statusword from Mock Motor ===");

    try {
        // Load motor configuration
        std::string config_path = get_motor_config_path();
        ObjectDictionary dict(config_path);
        INFO("✓ Loaded motor configuration from: " << config_path);

        // Create SDO client
        auto socket = create_test_socket("vcan_test");
        SDOClient sdo_client(socket, dict, dict.get_node_id());
        INFO("✓ Connected to vcan0, Node ID: " << static_cast<int>(dict.get_node_id()));

        // Read statusword (0x6041)
        INFO("Reading Statusword (0x6041)...");
        uint16_t statusword = sdo_client.read<uint16_t>("statusword");

        // Decode and display
        std::string state = decode_statusword_string(statusword);
        INFO("  Statusword: 0x" << std::hex << std::setw(4) << std::setfill('0') << statusword);
        INFO("  Motor State: " << state);

        // Check for common states (mock motor returns 0x0637 = ready to switch on)
        bool is_valid_state = (state != "UNKNOWN");
        REQUIRE(is_valid_state);

        // Additional info using cia402 helper functions
        if (has_fault(statusword)) {
            INFO("  ⚠ Fault detected in statusword");
        }
        if (voltage_enabled(statusword)) {
            INFO("  ✓ Voltage enabled");
        }
    } catch (const std::exception& e) {
        FAIL("SDO read failed: " << e.what());
    }
}

TEST_CASE_METHOD(CANopenIntegrationFixture, "SDO Integration: Read Error Register",
    "[integration][sdo][error]") {
    if (!is_vcan_test_available() || !is_motor_config_available()) {
        SKIP("Prerequisites not met");
    }

    INFO("=== Reading Error Register ===");

    try {
        std::string config_path = get_motor_config_path();
        ObjectDictionary dict(config_path);
        auto socket = create_test_socket("vcan_test");
        SDOClient sdo_client(socket, dict, dict.get_node_id());

        // Read error register (0x1001)
        INFO("Reading Error Register (0x1001)...");
        uint8_t error_reg = sdo_client.read<uint8_t>("error_register");

        INFO("  Error Register: 0x" << std::hex << std::setw(2) << std::setfill('0')
                                    << static_cast<int>(error_reg));

        if (error_reg == 0) {
            INFO("  ✓ No errors detected");
        } else {
            WARN("  Errors detected:");
            if (error_reg & ERR_GENERIC) WARN("    - Generic error");
            if (error_reg & ERR_CURRENT) WARN("    - Current error");
            if (error_reg & ERR_VOLTAGE) WARN("    - Voltage error");
            if (error_reg & ERR_TEMPERATURE) WARN("    - Temperature error");
            if (error_reg & ERR_COMMUNICATION) WARN("    - Communication error");
            if (error_reg & ERR_DEVICE_PROFILE) WARN("    - Device profile specific");
        }

        // Test passes regardless, just informational
        REQUIRE(true);

    } catch (const std::exception& e) {
        FAIL("SDO read failed: " << e.what());
    }
}

TEST_CASE_METHOD(CANopenIntegrationFixture, "SDO Integration: Read Mode of Operation",
    "[integration][sdo][mode]") {
    if (!is_vcan_test_available() || !is_motor_config_available()) {
        SKIP("Prerequisites not met");
    }

    INFO("=== Reading Mode of Operation ===");

    try {
        std::string config_path = get_motor_config_path();
        ObjectDictionary dict(config_path);
        auto socket = create_test_socket("vcan_test");
        SDOClient sdo_client(socket, dict, dict.get_node_id());

        // Read mode of operation display (0x6061)
        INFO("Reading Mode of Operation Display (0x6061)...");
        int8_t mode_display = sdo_client.read<int8_t>("modes_of_operation_display");

        INFO("  Current mode: " << static_cast<int>(mode_display));

        std::string mode_name;
        switch (mode_display) {
        case static_cast<int8_t>(OperationMode::PROFILE_POSITION): mode_name = "Profile Position";
            break;
        case static_cast<int8_t>(OperationMode::PROFILE_VELOCITY): mode_name = "Profile Velocity";
            break;
        case static_cast<int8_t>(OperationMode::TORQUE_PROFILE): mode_name = "Torque Profile";
            break;
        case static_cast<int8_t>(OperationMode::HOMING): mode_name = "Homing"; break;
        case static_cast<int8_t>(OperationMode::CYCLIC_SYNC_POSITION): mode_name =
                "Cyclic Sync Position"; break;
        case static_cast<int8_t>(OperationMode::CYCLIC_SYNC_VELOCITY): mode_name =
                "Cyclic Sync Velocity"; break;
        case static_cast<int8_t>(OperationMode::CYCLIC_SYNC_TORQUE): mode_name =
                "Cyclic Sync Torque"; break;
        default: mode_name = "Unknown/Not Set"; break;
        }
        INFO("  Mode Name: " << mode_name);

        // Verify it's a valid mode (1-10 or 0 if not set)
        REQUIRE(((mode_display >= 0 && mode_display <= 10) || mode_display == 0));

    } catch (const std::exception& e) {
        FAIL("SDO read failed: " << e.what());
    }
}

TEST_CASE_METHOD(CANopenIntegrationFixture, "SDO Integration: Read Position and Velocity",
    "[integration][sdo][feedback]") {
    if (!is_vcan_test_available() || !is_motor_config_available()) {
        SKIP("Prerequisites not met");
    }

    INFO("=== Reading Position and Velocity Feedback ===");

    try {
        std::string config_path = get_motor_config_path();
        ObjectDictionary dict(config_path);
        auto socket = create_test_socket("vcan_test");
        SDOClient sdo_client(socket, dict, dict.get_node_id());

        // Read position actual value (0x6064)
        INFO("Reading Position Actual Value (0x6064)...");
        int32_t position = sdo_client.read<int32_t>("position_actual");
        INFO("  Position: " << position << " counts");

        // Read velocity actual value (0x606C)
        INFO("Reading Velocity Actual Value (0x606C)...");
        int32_t velocity = sdo_client.read<int32_t>("velocity_actual");
        INFO("  Velocity: " << velocity << " rpm");

        // Both reads should succeed (values can be any int32)
        REQUIRE(true);

    } catch (const std::exception& e) {
        FAIL("SDO read failed: " << e.what());
    }
}

TEST_CASE_METHOD(CANopenIntegrationFixture, "SDO Integration: Write Mode of Operation (Safe)",
    "[integration][sdo][write]") {
    if (!is_vcan_test_available() || !is_motor_config_available()) {
        SKIP("Prerequisites not met");
    }

    INFO("=== Writing Mode of Operation ===");

    try {
        std::string config_path = get_motor_config_path();
        ObjectDictionary dict(config_path);
        auto socket = create_test_socket("vcan_test");
        SDOClient sdo_client(socket, dict, dict.get_node_id());

        // First check if motor is in fault
        uint16_t statusword = sdo_client.read<uint16_t>("statusword");
        if (has_fault(statusword)) {
            WARN("Motor is in FAULT state - skipping write test");
            SKIP("Motor in fault - cannot write mode");
        }

        // Read current mode
        int8_t current_mode = sdo_client.read<int8_t>("modes_of_operation_display");
        INFO("  Current mode: " << static_cast<int>(current_mode));

        // Write Profile Velocity mode (3)
        INFO("Writing Mode of Operation (0x6060) to Profile Velocity (3)...");
        bool write_ok = sdo_client.write<int8_t>("modes_of_operation",
            static_cast<int8_t>(OperationMode::PROFILE_VELOCITY));
        REQUIRE(write_ok);
        INFO("  ✓ Write successful");

        // Verify by reading back
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        int8_t verify_mode = sdo_client.read<int8_t>("modes_of_operation_display");
        INFO("  Verified mode: " << static_cast<int>(verify_mode));

        // Mode should be 3 (Profile Velocity)
        REQUIRE(verify_mode == static_cast<int8_t>(OperationMode::PROFILE_VELOCITY));
    } catch (const std::exception& e) {
        FAIL("SDO write/verify failed: " << e.what());
    }
}

TEST_CASE_METHOD(CANopenIntegrationFixture, "SDO Integration: Multiple Sequential Reads",
    "[integration][sdo][performance]") {
    if (!is_vcan_test_available() || !is_motor_config_available()) {
        SKIP("Prerequisites not met");
    }

    INFO("=== Testing Multiple Sequential SDO Reads ===");

    try {
        std::string config_path = get_motor_config_path();
        ObjectDictionary dict(config_path);
        auto socket = create_test_socket("vcan_test");
        SDOClient sdo_client(socket, dict, dict.get_node_id());

        const int num_reads = 5;
        INFO("Performing " << num_reads << " sequential reads of statusword...");

        auto start = std::chrono::steady_clock::now();

        for (int i = 0; i < num_reads; ++i) {
            uint16_t statusword = sdo_client.read<uint16_t>("statusword");
            INFO("  Read " << (i+1) << ": 0x" << std::hex << statusword);
        }

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        INFO("  Total time: " << duration.count() << " ms");
        INFO("  Average per read: " << (duration.count() / num_reads) << " ms");

        // Each read should take less than 500ms (SDO timeout is 1000ms)
        REQUIRE(duration.count() < (num_reads * 500));

    } catch (const std::exception& e) {
        FAIL("Sequential read test failed: " << e.what());
    }
}

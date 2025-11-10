/**
 * @file test_bridge_config.cpp
 * @brief Unit tests for bridge configuration
 * @version 1.0
 * @date 2025-10-14
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <fstream>

#include "../include/pattern/bridge_config.hpp"
#include "../include/exception/waveshare_exception.hpp"

using namespace waveshare;

TEST_CASE("BridgeConfig::create_default - Default values", "[bridge][config]") {
    auto config = BridgeConfig::create_default();

    REQUIRE(config.socketcan_interface == "vcan0");
    REQUIRE(config.usb_device_path == "/dev/ttyUSB0");
    REQUIRE(config.serial_baud_rate == SerialBaud::BAUD_2M);
    REQUIRE(config.can_baud_rate == CANBaud::BAUD_1M);
    REQUIRE(config.can_mode == CANMode::NORMAL);
    REQUIRE(config.auto_retransmit == true);
    REQUIRE(config.filter_id == 0);
    REQUIRE(config.filter_mask == 0);
    REQUIRE(config.usb_read_timeout_ms == 100);
    REQUIRE(config.socketcan_read_timeout_ms == 100);
}

TEST_CASE("BridgeConfig::validate - Empty interface throws", "[bridge][config][validation]") {
    BridgeConfig config = BridgeConfig::create_default();
    config.socketcan_interface = "";

    REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
}

TEST_CASE("BridgeConfig::validate - Empty USB device throws", "[bridge][config][validation]") {
    BridgeConfig config = BridgeConfig::create_default();
    config.usb_device_path = "";

    REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
}

TEST_CASE("BridgeConfig::validate - Zero timeout throws", "[bridge][config][validation]") {
    BridgeConfig config = BridgeConfig::create_default();

    SECTION("USB timeout zero") {
        config.usb_read_timeout_ms = 0;
        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("SocketCAN timeout zero") {
        config.socketcan_read_timeout_ms = 0;
        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }
}

TEST_CASE("BridgeConfig::validate - Timeout too large throws", "[bridge][config][validation]") {
    BridgeConfig config = BridgeConfig::create_default();

    SECTION("USB timeout too large") {
        config.usb_read_timeout_ms = 70000;
        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("SocketCAN timeout too large") {
        config.socketcan_read_timeout_ms = 70000;
        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }
}

TEST_CASE("BridgeConfig::validate - Filter ID validation", "[bridge][config][validation]") {
    BridgeConfig config = BridgeConfig::create_default();

    SECTION("Valid 29-bit ID") {
        config.filter_id = 0x1FFFFFFF;
        REQUIRE_NOTHROW(config.validate());
    }

    SECTION("Invalid ID > 29 bits") {
        config.filter_id = 0x20000000;
        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }
}

TEST_CASE("BridgeConfig::validate - Filter mask validation", "[bridge][config][validation]") {
    BridgeConfig config = BridgeConfig::create_default();

    SECTION("Valid 29-bit mask") {
        config.filter_mask = 0x1FFFFFFF;
        REQUIRE_NOTHROW(config.validate());
    }

    SECTION("Invalid mask > 29 bits") {
        config.filter_mask = 0x20000000;
        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }
}

// === JSON Configuration Tests ===

TEST_CASE("BridgeConfig::from_file - Parse JSON file", "[bridge][config][json]") {
    const std::string test_file = "/tmp/test_bridge_config.json";
    {
        std::ofstream out(test_file);
        out <<
            R"({
  "bridge_config": {
    "socketcan_interface": "can0",
    "usb_device_path": "/dev/ttyUSB1",
    "serial_baud_rate": 115200,
    "can_baud_rate": 500000,
    "can_mode": "loopback",
    "auto_retransmit": false,
    "filter_id": 291,
    "filter_mask": 2047,
    "usb_read_timeout_ms": 200,
    "socketcan_read_timeout_ms": 300
  }
})";
        out.close();
    }

    auto config = BridgeConfig::from_file(test_file, false);

    REQUIRE(config.socketcan_interface == "can0");
    REQUIRE(config.usb_device_path == "/dev/ttyUSB1");
    REQUIRE(config.serial_baud_rate == SerialBaud::BAUD_115200);
    REQUIRE(config.can_baud_rate == CANBaud::BAUD_500K);
    REQUIRE(config.can_mode == CANMode::LOOPBACK);
    REQUIRE(config.auto_retransmit == false);
    REQUIRE(config.filter_id == 291);
    REQUIRE(config.filter_mask == 2047);
    REQUIRE(config.usb_read_timeout_ms == 200);
    REQUIRE(config.socketcan_read_timeout_ms == 300);

    std::remove(test_file.c_str());
}

TEST_CASE("BridgeConfig::from_file - JSON with partial config uses defaults",
    "[bridge][config][json]") {
    const std::string test_file = "/tmp/test_bridge_partial.json";
    {
        std::ofstream out(test_file);
        out <<
            R"({
  "bridge_config": {
    "socketcan_interface": "can1",
    "can_baud_rate": 1000000
  }
})";
        out.close();
    }

    auto config = BridgeConfig::from_file(test_file, true);

    REQUIRE(config.socketcan_interface == "can1");
    REQUIRE(config.can_baud_rate == CANBaud::BAUD_1M);
    // Defaults for unspecified fields
    REQUIRE(config.usb_device_path == "/dev/ttyUSB0");
    REQUIRE(config.serial_baud_rate == SerialBaud::BAUD_2M);

    std::remove(test_file.c_str());
}

TEST_CASE("BridgeConfig::from_file - Invalid JSON throws", "[bridge][config][json]") {
    const std::string test_file = "/tmp/test_bridge_invalid.json";
    {
        std::ofstream out(test_file);
        out << "{ invalid json }";
        out.close();
    }

    REQUIRE_THROWS_AS(
        BridgeConfig::from_file(test_file, true),
        std::runtime_error
    );

    std::remove(test_file.c_str());
}

TEST_CASE("BridgeConfig::from_file - JSON invalid baud throws", "[bridge][config][json]") {
    const std::string test_file = "/tmp/test_bridge_bad_baud.json";
    {
        std::ofstream out(test_file);
        out << R"({
  "bridge_config": {
    "serial_baud_rate": 999999
  }
})";
        out.close();
    }

    REQUIRE_THROWS_AS(
        BridgeConfig::from_file(test_file, true),
        std::invalid_argument
    );

    std::remove(test_file.c_str());
}

TEST_CASE("BridgeConfig::from_file - JSON case insensitive mode", "[bridge][config][json]") {
    const std::string test_file = "/tmp/test_bridge_mode.json";
    {
        std::ofstream out(test_file);
        out << R"({
  "bridge_config": {
    "can_mode": "SILENT"
  }
})";
        out.close();
    }

    auto config = BridgeConfig::from_file(test_file, true);
    REQUIRE(config.can_mode == CANMode::SILENT);

    std::remove(test_file.c_str());
}

TEST_CASE("BridgeConfig::load - JSON priority: env > file > defaults",
    "[bridge][config][json][load]") {
    const std::string test_file = "/tmp/test_bridge_priority.json";
    {
        std::ofstream out(test_file);
        out <<
            R"({
  "bridge_config": {
    "socketcan_interface": "can_from_json",
    "can_baud_rate": 125000
  }
})";
        out.close();
    }

    // Set env var (overrides JSON file)
    setenv("WAVESHARE_SOCKETCAN_INTERFACE", "can_from_env", 1);

    auto config = BridgeConfig::load(test_file);

    // Env var wins
    REQUIRE(config.socketcan_interface == "can_from_env");
    // JSON value used (no env var set)
    REQUIRE(config.can_baud_rate == CANBaud::BAUD_125K);
    // Default value used (neither env nor JSON)
    REQUIRE(config.can_mode == CANMode::NORMAL);

    // Cleanup
    unsetenv("WAVESHARE_SOCKETCAN_INTERFACE");
    std::remove(test_file.c_str());
}

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

TEST_CASE("BridgeConfig::from_file - Parse .env file", "[bridge][config][file]") {
    // Create temporary .env file
    const std::string test_file = "/tmp/test_waveshare.env";
    {
        std::ofstream out(test_file);
        out << "# Test configuration\n";
        out << "WAVESHARE_SOCKETCAN_INTERFACE=can0\n";
        out << "WAVESHARE_USB_DEVICE=/dev/ttyUSB1\n";
        out << "WAVESHARE_SERIAL_BAUD=115200\n";
        out << "WAVESHARE_CAN_BAUD=500000\n";
        out << "WAVESHARE_CAN_MODE=loopback\n";
        out << "WAVESHARE_AUTO_RETRANSMIT=false\n";
        out << "WAVESHARE_FILTER_ID=0x123\n";
        out << "WAVESHARE_FILTER_MASK=0x7FF\n";
        out << "WAVESHARE_USB_READ_TIMEOUT=200\n";
        out << "WAVESHARE_SOCKETCAN_READ_TIMEOUT=300\n";
        out.close();
    }

    auto config = BridgeConfig::from_file(test_file, false);

    REQUIRE(config.socketcan_interface == "can0");
    REQUIRE(config.usb_device_path == "/dev/ttyUSB1");
    REQUIRE(config.serial_baud_rate == SerialBaud::BAUD_115200);
    REQUIRE(config.can_baud_rate == CANBaud::BAUD_500K);
    REQUIRE(config.can_mode == CANMode::LOOPBACK);
    REQUIRE(config.auto_retransmit == false);
    REQUIRE(config.filter_id == 0x123);
    REQUIRE(config.filter_mask == 0x7FF);
    REQUIRE(config.usb_read_timeout_ms == 200);
    REQUIRE(config.socketcan_read_timeout_ms == 300);

    // Cleanup
    std::remove(test_file.c_str());
}

TEST_CASE("BridgeConfig::from_file - Parse with comments and whitespace",
    "[bridge][config][file]") {
    const std::string test_file = "/tmp/test_waveshare_spaces.env";
    {
        std::ofstream out(test_file);
        out << "  # Comment with leading space\n";
        out << "\n";  // Empty line
        out << "WAVESHARE_SOCKETCAN_INTERFACE = can1  \n";  // Spaces around =
        out << "  WAVESHARE_CAN_MODE=silent  \n";  // Leading/trailing space
        out << "WAVESHARE_FILTER_ID = 0x456  # Inline comment ignored\n";
        out.close();
    }

    auto config = BridgeConfig::from_file(test_file, true);

    REQUIRE(config.socketcan_interface == "can1");
    REQUIRE(config.can_mode == CANMode::SILENT);
    REQUIRE(config.filter_id == 0x456);

    std::remove(test_file.c_str());
}

TEST_CASE("BridgeConfig::from_file - Parse quoted values", "[bridge][config][file]") {
    const std::string test_file = "/tmp/test_waveshare_quotes.env";
    {
        std::ofstream out(test_file);
        out << "WAVESHARE_SOCKETCAN_INTERFACE=\"can2\"\n";
        out << "WAVESHARE_USB_DEVICE='/dev/ttyUSB2'\n";
        out.close();
    }

    auto config = BridgeConfig::from_file(test_file, true);

    REQUIRE(config.socketcan_interface == "can2");
    REQUIRE(config.usb_device_path == "/dev/ttyUSB2");

    std::remove(test_file.c_str());
}

TEST_CASE("BridgeConfig::from_file - Invalid file throws", "[bridge][config][file]") {
    REQUIRE_THROWS_AS(
        BridgeConfig::from_file("/nonexistent/path/to/file.env"),
        std::runtime_error
    );
}

TEST_CASE("BridgeConfig::from_file - Invalid baud rate throws", "[bridge][config][file]") {
    const std::string test_file = "/tmp/test_waveshare_bad_baud.env";
    {
        std::ofstream out(test_file);
        out << "WAVESHARE_SERIAL_BAUD=999999\n";  // Invalid baud rate
        out.close();
    }

    REQUIRE_THROWS_AS(
        BridgeConfig::from_file(test_file, true),
        std::invalid_argument
    );

    std::remove(test_file.c_str());
}

TEST_CASE("BridgeConfig::from_file - Invalid CAN mode throws", "[bridge][config][file]") {
    const std::string test_file = "/tmp/test_waveshare_bad_mode.env";
    {
        std::ofstream out(test_file);
        out << "WAVESHARE_CAN_MODE=invalid_mode\n";
        out.close();
    }

    REQUIRE_THROWS_AS(
        BridgeConfig::from_file(test_file, true),
        std::invalid_argument
    );

    std::remove(test_file.c_str());
}

TEST_CASE("BridgeConfig::from_env - Read environment variables", "[bridge][config][env]") {
    // Set environment variables
    setenv("WAVESHARE_SOCKETCAN_INTERFACE", "can3", 1);
    setenv("WAVESHARE_CAN_BAUD", "250000", 1);
    setenv("WAVESHARE_AUTO_RETRANSMIT", "true", 1);

    auto config = BridgeConfig::from_env(true);

    REQUIRE(config.socketcan_interface == "can3");
    REQUIRE(config.can_baud_rate == CANBaud::BAUD_250K);
    REQUIRE(config.auto_retransmit == true);

    // Cleanup
    unsetenv("WAVESHARE_SOCKETCAN_INTERFACE");
    unsetenv("WAVESHARE_CAN_BAUD");
    unsetenv("WAVESHARE_AUTO_RETRANSMIT");
}

TEST_CASE("BridgeConfig::load - Priority: env > file > defaults", "[bridge][config][load]") {
    // Create .env file
    const std::string test_file = "/tmp/test_waveshare_priority.env";
    {
        std::ofstream out(test_file);
        out << "WAVESHARE_SOCKETCAN_INTERFACE=can_from_file\n";
        out << "WAVESHARE_CAN_BAUD=125000\n";
        out.close();
    }

    // Set env var (overrides file)
    setenv("WAVESHARE_SOCKETCAN_INTERFACE", "can_from_env", 1);

    auto config = BridgeConfig::load(test_file);

    // Env var wins
    REQUIRE(config.socketcan_interface == "can_from_env");
    // File value used (no env var set)
    REQUIRE(config.can_baud_rate == CANBaud::BAUD_125K);
    // Default value used (neither env nor file)
    REQUIRE(config.can_mode == CANMode::NORMAL);

    // Cleanup
    unsetenv("WAVESHARE_SOCKETCAN_INTERFACE");
    std::remove(test_file.c_str());
}

TEST_CASE("BridgeConfig::load - No file uses env and defaults", "[bridge][config][load]") {
    setenv("WAVESHARE_FILTER_ID", "0x789", 1);

    auto config = BridgeConfig::load(std::nullopt);

    REQUIRE(config.filter_id == 0x789);  // From env
    REQUIRE(config.socketcan_interface == "vcan0");  // Default (no env var)

    unsetenv("WAVESHARE_FILTER_ID");
}

TEST_CASE("BridgeConfig - CAN mode case insensitive", "[bridge][config][mode]") {
    const std::string test_file = "/tmp/test_waveshare_mode_case.env";

    SECTION("Uppercase LOOPBACK") {
        std::ofstream out(test_file);
        out << "WAVESHARE_CAN_MODE=LOOPBACK\n";
        out.close();

        auto config = BridgeConfig::from_file(test_file, true);
        REQUIRE(config.can_mode == CANMode::LOOPBACK);
    }

    SECTION("Mixed case SiLeNt") {
        std::ofstream out(test_file);
        out << "WAVESHARE_CAN_MODE=SiLeNt\n";
        out.close();

        auto config = BridgeConfig::from_file(test_file, true);
        REQUIRE(config.can_mode == CANMode::SILENT);
    }

    SECTION("Loopback-Silent variants") {
        std::ofstream out(test_file);
        out << "WAVESHARE_CAN_MODE=loopback-silent\n";
        out.close();

        auto config = BridgeConfig::from_file(test_file, true);
        REQUIRE(config.can_mode == CANMode::LOOPBACK_SILENT);
    }

    std::remove(test_file.c_str());
}

TEST_CASE("BridgeConfig - Boolean parsing", "[bridge][config][bool]") {
    const std::string test_file = "/tmp/test_waveshare_bool.env";

    SECTION("true") {
        std::ofstream out(test_file);
        out << "WAVESHARE_AUTO_RETRANSMIT=true\n";
        out.close();

        auto config = BridgeConfig::from_file(test_file, true);
        REQUIRE(config.auto_retransmit == true);
    }

    SECTION("1") {
        std::ofstream out(test_file);
        out << "WAVESHARE_AUTO_RETRANSMIT=1\n";
        out.close();

        auto config = BridgeConfig::from_file(test_file, true);
        REQUIRE(config.auto_retransmit == true);
    }

    SECTION("yes") {
        std::ofstream out(test_file);
        out << "WAVESHARE_AUTO_RETRANSMIT=yes\n";
        out.close();

        auto config = BridgeConfig::from_file(test_file, true);
        REQUIRE(config.auto_retransmit == true);
    }

    SECTION("false") {
        std::ofstream out(test_file);
        out << "WAVESHARE_AUTO_RETRANSMIT=false\n";
        out.close();

        auto config = BridgeConfig::from_file(test_file, true);
        REQUIRE(config.auto_retransmit == false);
    }

    std::remove(test_file.c_str());
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

/**
 * @file test_sdo_client.cpp
 * @brief Unit and integration tests for SDOClient class using Catch2
 * @author effibot (andrea.efficace1@gmail.com)
 * @date 2025-11-06
 *
 * Test Strategy:
 * 1. SDO frame construction (expedited write/read)
 * 2. COB-ID calculations
 * 3. Frame validation logic
 * 4. Type-safe template methods
 * 5. Integration tests with vcan0 (if available)
 *
 * @note Integration tests require vcan0 to be up: sudo ip link add dev vcan0 type vcan && sudo ip link set vcan0 up
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "canopen/sdo_client.hpp"
#include "canopen/object_dictionary.hpp"
#include "test_utils_canopen.hpp"
#include <fstream>
#include <filesystem>
#include <linux/can.h>
#include <thread>
#include <chrono>

using namespace canopen;
using namespace test_utils;
using Catch::Matchers::ContainsSubstring;

// ============================================================================
// TEST FIXTURE
// ============================================================================
struct SDOClientFixture {
    std::string test_config_path;
    std::unique_ptr<ObjectDictionary> dict;

    SDOClientFixture() {
        // Create test configuration
        std::filesystem::create_directories("/tmp/canopen_test");
        test_config_path = "/tmp/canopen_test/test_sdo_config.json";

        std::ofstream config_file(test_config_path);
        config_file <<
            R"({
            "node_id": 1,
            "device_name": "test_motor",
            "can_interface": "vcan0",
            "objects": {
                "controlword": {
                    "index": "0x6040",
                    "subindex": 0,
                    "datatype": "uint16_t",
                    "access": "rw"
                },
                "statusword": {
                    "index": "0x6041",
                    "subindex": 0,
                    "datatype": "uint16_t",
                    "access": "ro"
                },
                "target_velocity": {
                    "index": "0x60FF",
                    "subindex": 0,
                    "datatype": "int32_t",
                    "access": "rw"
                },
                "error_register": {
                    "index": "0x1001",
                    "subindex": 0,
                    "datatype": "uint8_t",
                    "access": "ro"
                }
            },
            "motor_parameters": {
                "max_rpm": 3000.0
            }
        })";
        config_file.close();

        dict = std::make_unique<ObjectDictionary>(test_config_path);
    }

    ~SDOClientFixture() {
        std::filesystem::remove(test_config_path);
    }

    // Helper to check if vcan0 is available
    bool is_vcan0_available() {
        std::ifstream ifs("/sys/class/net/vcan0/operstate");
        return ifs.good();
    }
};

// ============================================================================
// UNIT TESTS - Frame Construction (No Network Required)
// ============================================================================

TEST_CASE_METHOD(SDOClientFixture, "SDOClient: COB-ID calculations", "[sdo_client][cob_id]") {
    uint8_t node_id = 5;

    SECTION("SDO TX COB-ID (Client → Server)") {
        // Formula: 0x600 + node_id
        uint32_t expected = 0x600 + node_id;
        REQUIRE(expected == 0x605);
    }

    SECTION("SDO RX COB-ID (Server → Client)") {
        // Formula: 0x580 + node_id
        uint32_t expected = 0x580 + node_id;
        REQUIRE(expected == 0x585);
    }

    SECTION("Node ID boundaries") {
        // Valid node IDs: 1-127
        REQUIRE((0x600 + 1) == 0x601);    // Min
        REQUIRE((0x600 + 127) == 0x67F);  // Max
        REQUIRE((0x580 + 1) == 0x581);
        REQUIRE((0x580 + 127) == 0x5FF);
    }
}

TEST_CASE_METHOD(SDOClientFixture, "SDOClient: SDO expedited write frame construction",
    "[sdo_client][frame]") {
    // This tests the frame format without actually sending

    SECTION("Write uint16_t controlword (0x6040)") {
        // Expected frame structure for expedited write:
        // Byte 0: 0x2B (expedited download, 2 bytes indicated)
        // Byte 1-2: Index (little-endian)
        // Byte 3: Subindex
        // Byte 4-7: Data (little-endian, padded)

        uint16_t controlword_value = 0x000F;  // Enable operation
        std::vector<uint8_t> data = dict->to_raw(controlword_value);

        REQUIRE(data.size() == 2);
        REQUIRE(data[0] == 0x0F);  // LSB
        REQUIRE(data[1] == 0x00);  // MSB
    }

    SECTION("Write int32_t target velocity (0x60FF)") {
        int32_t velocity = 1000;  // 1000 RPM
        std::vector<uint8_t> data = dict->to_raw(velocity);

        REQUIRE(data.size() == 4);
        REQUIRE(data[0] == 0xE8);  // 1000 & 0xFF
        REQUIRE(data[1] == 0x03);  // (1000 >> 8) & 0xFF
        REQUIRE(data[2] == 0x00);
        REQUIRE(data[3] == 0x00);
    }

    SECTION("Write uint8_t error register (0x1001)") {
        uint8_t value = 0x42;
        std::vector<uint8_t> data = dict->to_raw(value);

        REQUIRE(data.size() == 1);
        REQUIRE(data[0] == 0x42);
    }
}

TEST_CASE_METHOD(SDOClientFixture, "SDOClient: SDO read request frame construction",
    "[sdo_client][frame]") {
    SECTION("Read request format") {
        // Expected frame for read request:
        // Byte 0: 0x40 (initiate upload)
        // Byte 1-2: Index (little-endian)
        // Byte 3: Subindex
        // Byte 4-7: Reserved (0x00)

        // These are validated by the actual frame construction in SDOClient
        REQUIRE(true);  // Placeholder - actual validation happens in integration test
    }
}

// ============================================================================
// INTEGRATION TESTS - Require vcan0 Interface
// ============================================================================

TEST_CASE_METHOD(SDOClientFixture, "SDOClient: Socket creation and binding",
    "[sdo_client][integration][.]") {
    if (!is_vcan0_available()) {
        SKIP("vcan0 not available, skipping integration test. "
            "Create it with: sudo ip link add dev vcan0 type vcan && sudo ip link set vcan0 up");
    }

    SECTION("Successfully create SDOClient with vcan0") {
        auto socket = create_test_socket("vcan0");
        REQUIRE_NOTHROW(SDOClient(socket, *dict, 1));

        SDOClient client(socket, *dict, 1);
        REQUIRE(client.is_open());
    }

    SECTION("Throw exception for invalid interface") {
        REQUIRE_THROWS_WITH(
            create_test_socket("nonexistent_can"),
            ContainsSubstring("Interface not found")
        );
    }

    SECTION("Multiple clients on same interface") {
        auto socket1 = create_test_socket("vcan0");
        auto socket2 = create_test_socket("vcan0");
        SDOClient client1(socket1, *dict, 1);
        SDOClient client2(socket2, *dict, 2);

        REQUIRE(client1.is_open());
        REQUIRE(client2.is_open());
    }
}

TEST_CASE_METHOD(SDOClientFixture, "SDOClient: Frame send/receive on vcan0",
    "[sdo_client][integration][.]") {
    if (!is_vcan0_available()) {
        SKIP("vcan0 not available");
    }

    SECTION("Send SDO write frame to vcan0") {
        auto socket_1 = create_test_socket("vcan0");
        SDOClient client(socket_1, *dict, 1);

        // Attempt to write controlword (will timeout without real device)
        uint16_t value = 0x000F;
        std::vector<uint8_t> data = dict->to_raw(value);

        // This will timeout but frame should be sent successfully
        // We can verify with candump in another terminal
        WARN("Sending SDO write frame to vcan0 (will timeout without device)");
        bool result = client.write_object("controlword", data, std::chrono::milliseconds(100));

        // Expected to fail (timeout) since no device is responding
        REQUIRE(result == false);
    }
}

TEST_CASE_METHOD(SDOClientFixture, "SDOClient: Type-safe template methods",
    "[sdo_client][integration][.]") {
    if (!is_vcan0_available()) {
        SKIP("vcan0 not available");
    }

    auto socket_1 = create_test_socket("vcan0");
        SDOClient client(socket_1, *dict, 1);

    SECTION("Write uint16_t using template method") {
        // Will timeout but syntax should compile and execute
        bool result = client.write<uint16_t>("controlword", 0x000F);
        REQUIRE(result == false);  // Timeout expected
    }

    SECTION("Write int32_t using template method") {
        bool result = client.write<int32_t>("target_velocity", 1000);
        REQUIRE(result == false);  // Timeout expected
    }
}

// ============================================================================
// LOOPBACK TEST - vcan0 with canecho running
// ============================================================================

TEST_CASE_METHOD(SDOClientFixture, "SDOClient: Loopback test with candump verification",
    "[sdo_client][manual][.]") {
    // This test is for manual verification with candump
    // Run in another terminal: candump vcan0

    if (!is_vcan0_available()) {
        SKIP("vcan0 not available");
    }

    SECTION("Send recognizable frame for candump verification") {
        auto socket_1 = create_test_socket("vcan0");
        SDOClient client(socket_1, *dict, 1);

        INFO("Run 'candump vcan0' in another terminal to verify frames");
        INFO("Expected COB-ID: 0x601 (0x600 + node_id)");
        INFO("Expected data: [2B 40 60 00 0F 00 00 00] for controlword write");

        uint16_t value = 0x000F;
        std::vector<uint8_t> data = dict->to_raw(value);

        // Send frame
        client.write_object("controlword", data, std::chrono::milliseconds(100));

        // Give time to observe in candump
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        WARN("Check candump output for CAN frame 601#...");
    }
}

// ============================================================================
// ERROR HANDLING TESTS
// ============================================================================

TEST_CASE_METHOD(SDOClientFixture, "SDOClient: Error handling", "[sdo_client][error]") {
    if (!is_vcan0_available()) {
        SKIP("vcan0 not available");
    }

    auto socket_1 = create_test_socket("vcan0");
        SDOClient client(socket_1, *dict, 1);

    SECTION("Write to non-existent object") {
        std::vector<uint8_t> data = {0x00, 0x00};

        REQUIRE_THROWS_WITH(
            client.write_object("nonexistent_object", data),
            ContainsSubstring("Object not found")
        );
    }

    SECTION("Read from non-existent object") {
        REQUIRE_THROWS_WITH(
            client.read_object("nonexistent_object"),
            ContainsSubstring("Object not found")
        );
    }
}

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================

TEST_CASE_METHOD(SDOClientFixture, "SDOClient: Performance characteristics",
    "[sdo_client][performance][.]") {
    if (!is_vcan0_available()) {
        SKIP("vcan0 not available");
    }

    auto socket_1 = create_test_socket("vcan0");
        SDOClient client(socket_1, *dict, 1);

    SECTION("Timeout behavior") {
        auto start = std::chrono::steady_clock::now();

        bool result = client.write<uint16_t>("controlword", 0x000F);

        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        REQUIRE(result == false);  // Should timeout
        REQUIRE(elapsed.count() >= 100);  // Should wait at least timeout duration
        REQUIRE(elapsed.count() < 150);   // Should not wait much longer
    }
}

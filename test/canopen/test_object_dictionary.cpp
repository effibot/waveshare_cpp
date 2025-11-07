/**
 * @file test_object_dictionary.cpp
 * @brief Unit tests for ObjectDictionary class using Catch2
 * @author effibot (andrea.efficace1@gmail.com)
 * @date 2025-11-06
 *
 * Test Strategy:
 * 1. JSON parsing and configuration loading
 * 2. Object lookup by name
 * 3. Data type conversions (to_raw / from_raw)
 * 4. PDO object retrieval
 * 5. Motor parameter access
 * 6. Error handling (missing objects, invalid types)
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "canopen/object_dictionary.hpp"
#include <fstream>
#include <filesystem>

using namespace canopen;
using Catch::Matchers::ContainsSubstring;

// ============================================================================
// TEST FIXTURE - Create temporary config file for testing
// ============================================================================
struct ObjectDictionaryFixture {
    std::string test_config_path;

    ObjectDictionaryFixture() {
        // Create temporary directory if it doesn't exist
        std::filesystem::create_directories("/tmp/canopen_test");
        test_config_path = "/tmp/canopen_test/test_motor_config.json";

        // Create minimal test configuration
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
                    "access": "rw",
                    "pdo_mapping": "rpdo1"
                },
                "statusword": {
                    "index": "0x6041",
                    "subindex": 0,
                    "datatype": "uint16_t",
                    "access": "ro",
                    "pdo_mapping": "tpdo1"
                },
                "target_velocity": {
                    "index": "0x60FF",
                    "subindex": 0,
                    "datatype": "int32_t",
                    "access": "rw",
                    "pdo_mapping": "rpdo2",
                    "unit": "rpm",
                    "scaling_factor": 1.0
                },
                "velocity_actual": {
                    "index": "0x606C",
                    "subindex": 0,
                    "datatype": "int32_t",
                    "access": "ro",
                    "pdo_mapping": "tpdo2",
                    "unit": "rpm"
                },
                "modes_of_operation": {
                    "index": "0x6060",
                    "subindex": 0,
                    "datatype": "int8_t",
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
                "max_rpm": 3000.0,
                "encoder_resolution": 2048.0,
                "wheel_radius_m": 0.1
            }
        })";
        config_file.close();
    }

    ~ObjectDictionaryFixture() {
        // Cleanup
        std::filesystem::remove(test_config_path);
    }
};

// ============================================================================
// TEST CASES
// ============================================================================

TEST_CASE_METHOD(ObjectDictionaryFixture, "ObjectDictionary: JSON parsing",
    "[object_dictionary][parsing]") {
    SECTION("Successfully parse valid JSON config") {
        REQUIRE_NOTHROW(ObjectDictionary(test_config_path));

        ObjectDictionary dict(test_config_path);
        REQUIRE(dict.get_node_id() == 1);
        REQUIRE(dict.get_device_name() == "test_motor");
        REQUIRE(dict.get_can_interface() == "vcan0");
    }

    SECTION("Throw exception for non-existent file") {
        REQUIRE_THROWS_AS(
            ObjectDictionary("/nonexistent/path.json"),
            std::runtime_error
        );
    }

    SECTION("Throw exception for invalid JSON") {
        std::string invalid_path = "/tmp/canopen_test/invalid.json";
        std::ofstream invalid_file(invalid_path);
        invalid_file << "{ invalid json content ";
        invalid_file.close();

        REQUIRE_THROWS_AS(
            ObjectDictionary(invalid_path),
            std::runtime_error
        );

        std::filesystem::remove(invalid_path);
    }
}

TEST_CASE_METHOD(ObjectDictionaryFixture, "ObjectDictionary: Object lookup",
    "[object_dictionary][lookup]") {
    ObjectDictionary dict(test_config_path);

    SECTION("Get existing object by name") {
        auto controlword = dict.get_object("controlword");
        REQUIRE(controlword.index == 0x6040);
        REQUIRE(controlword.subindex == 0);
        REQUIRE(controlword.datatype == ObjectDictionary::DataType::UINT16);
        REQUIRE(controlword.access == "rw");
        REQUIRE(controlword.pdo_mapping == "rpdo1");
    }

    SECTION("Check object existence") {
        REQUIRE(dict.has_object("controlword") == true);
        REQUIRE(dict.has_object("statusword") == true);
        REQUIRE(dict.has_object("nonexistent") == false);
    }

    SECTION("Throw exception for non-existent object") {
        REQUIRE_THROWS_WITH(
            dict.get_object("nonexistent"),
            ContainsSubstring("Object not found")
        );
    }

    SECTION("Verify all expected objects are present") {
        REQUIRE(dict.has_object("controlword"));
        REQUIRE(dict.has_object("statusword"));
        REQUIRE(dict.has_object("target_velocity"));
        REQUIRE(dict.has_object("velocity_actual"));
        REQUIRE(dict.has_object("modes_of_operation"));
        REQUIRE(dict.has_object("error_register"));
    }
}

TEST_CASE_METHOD(ObjectDictionaryFixture, "ObjectDictionary: Object properties",
    "[object_dictionary][properties]") {
    ObjectDictionary dict(test_config_path);

    SECTION("Verify controlword properties") {
        auto obj = dict.get_object("controlword");
        REQUIRE(obj.index == 0x6040);
        REQUIRE(obj.size_bytes() == 2);
        REQUIRE(obj.datatype == ObjectDictionary::DataType::UINT16);
    }

    SECTION("Verify target_velocity properties") {
        auto obj = dict.get_object("target_velocity");
        REQUIRE(obj.index == 0x60FF);
        REQUIRE(obj.size_bytes() == 4);
        REQUIRE(obj.datatype == ObjectDictionary::DataType::INT32);
        REQUIRE(obj.unit == "rpm");
        REQUIRE(obj.scaling_factor == 1.0);
    }

    SECTION("Verify modes_of_operation properties") {
        auto obj = dict.get_object("modes_of_operation");
        REQUIRE(obj.index == 0x6060);
        REQUIRE(obj.size_bytes() == 1);
        REQUIRE(obj.datatype == ObjectDictionary::DataType::INT8);
    }
}

TEST_CASE_METHOD(ObjectDictionaryFixture, "ObjectDictionary: Data type conversions",
    "[object_dictionary][conversion]") {
    ObjectDictionary dict(test_config_path);

    SECTION("Convert uint16_t to raw bytes (little-endian)") {
        uint16_t value = 0x1234;
        auto raw = dict.to_raw(value);

        REQUIRE(raw.size() == 2);
        REQUIRE(raw[0] == 0x34);  // LSB first
        REQUIRE(raw[1] == 0x12);  // MSB last
    }

    SECTION("Convert int32_t to raw bytes (little-endian)") {
        int32_t value = 0x12345678;
        auto raw = dict.to_raw(value);

        REQUIRE(raw.size() == 4);
        REQUIRE(raw[0] == 0x78);
        REQUIRE(raw[1] == 0x56);
        REQUIRE(raw[2] == 0x34);
        REQUIRE(raw[3] == 0x12);
    }

    SECTION("Convert uint8_t to raw bytes") {
        uint8_t value = 0xAB;
        auto raw = dict.to_raw(value);

        REQUIRE(raw.size() == 1);
        REQUIRE(raw[0] == 0xAB);
    }

    SECTION("Convert raw bytes to uint16_t") {
        std::vector<uint8_t> raw = {0x34, 0x12};
        uint16_t value = dict.from_raw<uint16_t>(raw);

        REQUIRE(value == 0x1234);
    }

    SECTION("Convert raw bytes to int32_t") {
        std::vector<uint8_t> raw = {0x78, 0x56, 0x34, 0x12};
        int32_t value = dict.from_raw<int32_t>(raw);

        REQUIRE(value == 0x12345678);
    }

    SECTION("Convert negative int32_t") {
        int32_t value = -1000;
        auto raw = dict.to_raw(value);
        int32_t recovered = dict.from_raw<int32_t>(raw);

        REQUIRE(recovered == value);
    }

    SECTION("Round-trip conversion preserves value") {
        uint16_t original = 0xABCD;
        auto raw = dict.to_raw(original);
        uint16_t recovered = dict.from_raw<uint16_t>(raw);

        REQUIRE(recovered == original);
    }

    SECTION("Throw exception for insufficient data") {
        std::vector<uint8_t> insufficient_data = {0x12};  // Only 1 byte

        REQUIRE_THROWS_WITH(
            dict.from_raw<uint16_t>(insufficient_data),
            ContainsSubstring("Insufficient data")
        );
    }
}

TEST_CASE_METHOD(ObjectDictionaryFixture, "ObjectDictionary: PDO object retrieval",
    "[object_dictionary][pdo]") {
    ObjectDictionary dict(test_config_path);

    SECTION("Get objects mapped to RPDO1") {
        auto rpdo1_objects = dict.get_pdo_objects("rpdo1");
        REQUIRE(rpdo1_objects.size() == 1);
        REQUIRE(rpdo1_objects[0] == "controlword");
    }

    SECTION("Get objects mapped to RPDO2") {
        auto rpdo2_objects = dict.get_pdo_objects("rpdo2");
        REQUIRE(rpdo2_objects.size() == 1);
        REQUIRE(rpdo2_objects[0] == "target_velocity");
    }

    SECTION("Get objects mapped to TPDO1") {
        auto tpdo1_objects = dict.get_pdo_objects("tpdo1");
        REQUIRE(tpdo1_objects.size() == 1);
        REQUIRE(tpdo1_objects[0] == "statusword");
    }

    SECTION("Get objects mapped to TPDO2") {
        auto tpdo2_objects = dict.get_pdo_objects("tpdo2");
        REQUIRE(tpdo2_objects.size() == 1);
        REQUIRE(tpdo2_objects[0] == "velocity_actual");
    }

    SECTION("Return empty vector for non-existent PDO") {
        auto objects = dict.get_pdo_objects("nonexistent_pdo");
        REQUIRE(objects.empty());
    }
}

TEST_CASE_METHOD(ObjectDictionaryFixture, "ObjectDictionary: Motor parameters",
    "[object_dictionary][motor_params]") {
    ObjectDictionary dict(test_config_path);

    SECTION("Get existing motor parameters") {
        REQUIRE(dict.get_motor_param("max_rpm") == 3000.0);
        REQUIRE(dict.get_motor_param("encoder_resolution") == 2048.0);
        REQUIRE(dict.get_motor_param("wheel_radius_m") == 0.1);
    }

    SECTION("Throw exception for non-existent parameter") {
        REQUIRE_THROWS_WITH(
            dict.get_motor_param("nonexistent_param"),
            ContainsSubstring("Motor parameter not found")
        );
    }
}

TEST_CASE_METHOD(ObjectDictionaryFixture, "ObjectDictionary: Size calculations",
    "[object_dictionary][size]") {
    ObjectDictionary dict(test_config_path);

    SECTION("UINT8/INT8 size is 1 byte") {
        auto obj = dict.get_object("error_register");
        REQUIRE(obj.size_bytes() == 1);
    }

    SECTION("UINT16/INT16 size is 2 bytes") {
        auto obj = dict.get_object("controlword");
        REQUIRE(obj.size_bytes() == 2);
    }

    SECTION("INT32/UINT32 size is 4 bytes") {
        auto obj = dict.get_object("target_velocity");
        REQUIRE(obj.size_bytes() == 4);
    }
}

// ============================================================================
// INTEGRATION TEST - Use actual motor_config.json
// ============================================================================
TEST_CASE("ObjectDictionary: Load actual motor config", "[object_dictionary][integration]") {
    // This test uses the actual motor_config.json from the project
    std::string config_path = "../config/motor_config.json";

    if (std::filesystem::exists(config_path)) {
        SECTION("Load and verify actual configuration") {
            ObjectDictionary dict(config_path);

            REQUIRE(dict.get_node_id() == 1);
            REQUIRE(dict.get_device_name() == "traction_motor_left");
            REQUIRE(dict.get_can_interface() == "vcan0");

            // Verify critical objects exist
            REQUIRE(dict.has_object("controlword"));
            REQUIRE(dict.has_object("statusword"));
            REQUIRE(dict.has_object("target_velocity"));
            REQUIRE(dict.has_object("velocity_actual"));
            REQUIRE(dict.has_object("position_actual"));

            // Verify motor parameters
            REQUIRE_NOTHROW(dict.get_motor_param("max_rpm"));
            REQUIRE_NOTHROW(dict.get_motor_param("encoder_resolution"));
        }
    } else {
        WARN("Actual motor_config.json not found at " << config_path <<
            ", skipping integration test");
    }
}

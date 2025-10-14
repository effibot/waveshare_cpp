/**
 * @file test_fixed_frame.cpp
 * @brief Focused unit tests for FixedFrame class using Catch2
 *
 * Test Strategy (State-First Architecture):
 * 1. Individual getter/setter pairs (ID, format, DLC, data)
 * 2. Constructor parameter validation
 * 3. Checksum integrity via serialization
 * 4. Round-trip serialization/deserialization
 * 5. Standard vs Extended ID handling
 *
 * @note Build: cd build && cmake .. && cmake --build .
 * @note Run: ./test_fixed_frame
 */

#include <catch2/catch_test_macros.hpp>
#include "../include/frame/fixed_frame.hpp"
#include "../include/interface/serialization_helpers.hpp"
#include <array>
#include <vector>

using namespace waveshare;

// ============================================================================
// TEST FIXTURE - Minimal shared setup
// ============================================================================
struct FixedFrameFixture {
    // Predefined frame dump from protocol documentation (20 bytes fixed)
    // AA 55 01 01 01 23 01 00 00 08 11 22 33 44 55 66 77 88 00 XX
    // Note: ID is stored in little-endian format
    static constexpr std::array<std::uint8_t, 20> KNOWN_FRAME_DUMP = {
        0xAA,  // [0]  START
        0x55,  // [1]  HEADER
        0x01,  // [2]  TYPE (DATA_FIXED)
        0x01,  // [3]  CAN_VERS (STD_FIXED = 0x01)
        0x01,  // [4]  FORMAT (DATA_FIXED = 0x01)
        0x23,  // [5]  ID[0] (little-endian, LSB first)
        0x01,  // [6]  ID[1]
        0x00,  // [7]  ID[2]
        0x00,  // [8]  ID[3] (MSB last)
        0x08,  // [9]  DLC (8 bytes)
        0x11,  // [10] DATA[0]
        0x22,  // [11] DATA[1]
        0x33,  // [12] DATA[2]
        0x44,  // [13] DATA[3]
        0x55,  // [14] DATA[4]
        0x66,  // [15] DATA[5]
        0x77,  // [16] DATA[6]
        0x88,  // [17] DATA[7]
        0x00,  // [18] RESERVED
        0x93   // [19] CHECKSUM (calculated for little-endian ID)
    };

    // Expected values from the dump above
    static constexpr uint32_t EXPECTED_ID = 0x00000123;  // Little-endian bytes: 23 01 00 00
    static constexpr std::size_t EXPECTED_DLC = 8;
    static constexpr std::array<std::uint8_t, 8> EXPECTED_DATA = {
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
    };
    static constexpr Format EXPECTED_FORMAT = Format::DATA_FIXED;
    static constexpr CANVersion EXPECTED_VERSION = CANVersion::STD_FIXED;
    static constexpr std::uint8_t EXPECTED_CHECKSUM = 0x93;  // Correct checksum for little-endian ID
};

// ============================================================================
// CONSTRUCTOR TESTS
// ============================================================================

TEST_CASE("FixedFrame - Default constructor initializes correctly", "[constructor]") {
    FixedFrame frame;

    // Verify defaults
    REQUIRE(frame.get_can_id() == 0x00000000);
    REQUIRE(frame.get_dlc() == 0);
    REQUIRE(frame.get_format() == Format::DATA_FIXED);
    REQUIRE(frame.is_extended() == false);  // STD_FIXED
    REQUIRE(frame.is_remote() == false);
    REQUIRE(frame.size() == 20);  // Fixed 20 bytes
}

TEST_CASE("FixedFrame - Parameterized constructor sets all fields correctly", "[constructor]") {
    // Create test data
    std::uint32_t test_id = 0x12345678;
    std::vector<std::uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

    FixedFrame frame(
        Format::DATA_FIXED,
        CANVersion::EXT_FIXED,
        test_id,
        span<const std::uint8_t>(data.data(), data.size())
    );

    REQUIRE(frame.get_can_id() == 0x12345678);
    REQUIRE(frame.get_dlc() == 8);
    REQUIRE(frame.get_format() == Format::DATA_FIXED);
    REQUIRE(frame.is_extended() == true);  // EXT_FIXED

    auto frame_data = frame.get_data();
    for (size_t i = 0; i < 8; ++i) {
        REQUIRE(frame_data[i] == data[i]);
    }
}

// ============================================================================
// GETTER/SETTER TESTS - Individual field operations
// ============================================================================

TEST_CASE("FixedFrame - CAN ID getter/setter round-trip", "[getter][setter][id]") {
    SECTION("Standard CAN ID boundaries") {
        FixedFrame frame;  // Default is STD_FIXED
        frame.set_id(0x000);
        REQUIRE(frame.get_can_id() == 0x000);

        frame.set_id(0x7FF);  // Max 11-bit
        REQUIRE(frame.get_can_id() == 0x7FF);
    }

    SECTION("Extended CAN ID boundaries") {
        FixedFrame frame(Format::DATA_FIXED, CANVersion::EXT_FIXED, 0x00000000);
        frame.set_id(0x00000000);
        REQUIRE(frame.get_can_id() == 0x00000000);

        frame.set_id(0x1FFFFFFF);  // Max 29-bit
        REQUIRE(frame.get_can_id() == 0x1FFFFFFF);
    }

    SECTION("Arbitrary ID with endianness preservation") {
        FixedFrame frame(Format::DATA_FIXED, CANVersion::EXT_FIXED, 0x00000000);
        frame.set_id(0x12345678);
        REQUIRE(frame.get_can_id() == 0x12345678);

        frame.set_id(0x1ABBCCDD);
        REQUIRE(frame.get_can_id() == 0x1ABBCCDD);
    }
}

TEST_CASE("FixedFrame - Format getter/setter round-trip", "[getter][setter][format]") {
    FixedFrame frame;

    frame.set_format(Format::DATA_FIXED);
    REQUIRE(frame.get_format() == Format::DATA_FIXED);
    REQUIRE(frame.is_remote() == false);

    frame.set_format(Format::REMOTE_FIXED);
    REQUIRE(frame.get_format() == Format::REMOTE_FIXED);
    REQUIRE(frame.is_remote() == true);
}

TEST_CASE("FixedFrame - Data getter/setter round-trip", "[getter][setter][data]") {
    FixedFrame frame;

    SECTION("Full 8-byte data payload") {
        std::array<std::uint8_t, 8> data = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
        frame.set_data(span<const std::uint8_t>(data.data(), 8));

        REQUIRE(frame.get_dlc() == 8);
        auto retrieved = frame.get_data();
        for (size_t i = 0; i < 8; ++i) {
            REQUIRE(retrieved[i] == data[i]);
        }
    }

    SECTION("Partial data payload (DLC < 8)") {
        std::array<std::uint8_t, 4> data = {0x01, 0x02, 0x03, 0x04};
        frame.set_data(span<const std::uint8_t>(data.data(), 4));

        REQUIRE(frame.get_dlc() == 4);
        auto retrieved = frame.get_data();
        for (size_t i = 0; i < 4; ++i) {
            REQUIRE(retrieved[i] == data[i]);
        }
    }

    SECTION("Empty data payload (DLC = 0)") {
        std::array<std::uint8_t, 0> data = {};
        frame.set_data(span<const std::uint8_t>(data.data(), 0));

        REQUIRE(frame.get_dlc() == 0);
    }
}

TEST_CASE("FixedFrame - DLC automatically updates with data", "[getter][setter][dlc]") {
    FixedFrame frame;

    std::array<std::uint8_t, 5> data = {0x10, 0x20, 0x30, 0x40, 0x50};
    frame.set_data(span<const std::uint8_t>(data.data(), 5));

    // DLC should auto-update to match data size
    REQUIRE(frame.get_dlc() == 5);
}

TEST_CASE("FixedFrame - Extended vs Standard ID detection", "[getter][setter][extended]") {
    SECTION("Standard ID frame") {
        FixedFrame std_frame(Format::DATA_FIXED, CANVersion::STD_FIXED, 0x123);
        REQUIRE(std_frame.is_extended() == false);
    }

    SECTION("Extended ID frame") {
        FixedFrame ext_frame(Format::DATA_FIXED, CANVersion::EXT_FIXED, 0x12345678);
        REQUIRE(ext_frame.is_extended() == true);
    }
}

TEST_CASE("FixedFrame - set_id validates ID against CAN version", "[setter][validation][id]") {
    SECTION("Standard ID frame - valid IDs accepted") {
        FixedFrame std_frame(Format::DATA_FIXED, CANVersion::STD_FIXED, 0x000);

        // Should accept valid standard IDs (0x000 to 0x7FF)
        REQUIRE_NOTHROW(std_frame.set_id(0x000));
        REQUIRE_NOTHROW(std_frame.set_id(0x7FF));
        REQUIRE_NOTHROW(std_frame.set_id(0x400));

        REQUIRE(std_frame.get_can_id() == 0x400);
    }

    SECTION("Standard ID frame - invalid IDs rejected") {
        FixedFrame std_frame(Format::DATA_FIXED, CANVersion::STD_FIXED, 0x000);

        // Should reject IDs > 0x7FF (11-bit max)
        REQUIRE_THROWS_AS(std_frame.set_id(0x800), std::out_of_range);
        REQUIRE_THROWS_AS(std_frame.set_id(0xFFF), std::out_of_range);
        REQUIRE_THROWS_AS(std_frame.set_id(0x12345678), std::out_of_range);

        // Verify ID unchanged after exception
        REQUIRE(std_frame.get_can_id() == 0x000);
    }

    SECTION("Extended ID frame - valid IDs accepted") {
        FixedFrame ext_frame(Format::DATA_FIXED, CANVersion::EXT_FIXED, 0x00000000);

        // Should accept valid extended IDs (0x00000000 to 0x1FFFFFFF)
        REQUIRE_NOTHROW(ext_frame.set_id(0x00000000));
        REQUIRE_NOTHROW(ext_frame.set_id(0x1FFFFFFF));
        REQUIRE_NOTHROW(ext_frame.set_id(0x12345678));

        REQUIRE(ext_frame.get_can_id() == 0x12345678);
    }

    SECTION("Extended ID frame - invalid IDs rejected") {
        FixedFrame ext_frame(Format::DATA_FIXED, CANVersion::EXT_FIXED, 0x00000000);

        // Should reject IDs > 0x1FFFFFFF (29-bit max)
        REQUIRE_THROWS_AS(ext_frame.set_id(0x20000000), std::out_of_range);
        REQUIRE_THROWS_AS(ext_frame.set_id(0xFFFFFFFF), std::out_of_range);

        // Verify ID unchanged after exception
        REQUIRE(ext_frame.get_can_id() == 0x00000000);
    }
}

TEST_CASE("FixedFrame - Field independence (setters don't interfere)",
    "[getter][setter][independence]") {
    // Use extended frame to allow large ID values
    FixedFrame frame(Format::DATA_FIXED, CANVersion::EXT_FIXED, 0x00000000);

    // Set all fields
    frame.set_id(0x12345678);
    frame.set_format(Format::DATA_FIXED);
    std::array<std::uint8_t, 8> data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    frame.set_data(span<const std::uint8_t>(data.data(), 8));

    // Verify all retain values
    REQUIRE(frame.get_can_id() == 0x12345678);
    REQUIRE(frame.get_format() == Format::DATA_FIXED);
    REQUIRE(frame.get_dlc() == 8);

    // Change one field
    frame.set_id(0x1ABBCCDD);

    // Verify others unchanged
    REQUIRE(frame.get_format() == Format::DATA_FIXED);
    REQUIRE(frame.get_dlc() == 8);
}

// ============================================================================
// CHECKSUM TESTS - Integrity and dirty bit behavior
// ============================================================================

TEST_CASE("FixedFrame - Checksum calculation via serialization",
    "[checksum][integrity]") {
    FixedFrame frame;

    frame.set_id(0x123);
    std::array<std::uint8_t, 4> data = {0xAA, 0xBB, 0xCC, 0xDD};
    frame.set_data(span<const std::uint8_t>(data.data(), 4));

    // Serialize to get buffer with checksum
    auto buffer = frame.serialize();

    // Verify checksum is correct using ChecksumHelper
    REQUIRE(buffer.size() == 20);
    REQUIRE(ChecksumHelper::validate(buffer, 19, 2, 19) == true);
}

TEST_CASE("FixedFrame - Checksum updates after field modifications",
    "[checksum][dirty]") {
    FixedFrame frame;

    auto buffer1 = frame.serialize();
    std::uint8_t checksum1 = buffer1[19];

    // Modify field
    frame.set_id(0x456);
    auto buffer2 = frame.serialize();
    std::uint8_t checksum2 = buffer2[19];

    REQUIRE(checksum1 != checksum2);
}

TEST_CASE("FixedFrame - Checksum verification via serialization", "[checksum][verify]") {
    FixedFrame frame;

    frame.set_id(0x123);
    std::array<std::uint8_t, 8> data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    frame.set_data(span<const std::uint8_t>(data.data(), 8));

    auto buffer = frame.serialize();
    REQUIRE(ChecksumHelper::validate(buffer, 19, 2, 19) == true);
}

TEST_CASE("FixedFrame - Static checksum helper works on external data",
    "[checksum][static]") {
    std::vector<std::uint8_t> data = {
        0xAA, 0x55, 0x01, 0x01, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00  // Checksum placeholder
    };

    std::uint8_t checksum = ChecksumHelper::compute(data, 2, 19);
    data[19] = checksum;

    REQUIRE(ChecksumHelper::validate(data, 19, 2, 19) == true);
}

// ============================================================================
// ROUND-TRIP TESTS - Serialization/Deserialization
// ============================================================================

TEST_CASE_METHOD(FixedFrameFixture,
    "FixedFrame - Round-trip from known frame dump", "[roundtrip][integration]") {

    std::array<std::uint8_t, 8> data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

    FixedFrame frame(
        EXPECTED_FORMAT,
        EXPECTED_VERSION,
        EXPECTED_ID,
        span<const std::uint8_t>(data.data(), EXPECTED_DLC)
    );

    // Field-by-field verification
    REQUIRE(frame.get_can_id() == EXPECTED_ID);
    REQUIRE(frame.get_dlc() == EXPECTED_DLC);
    REQUIRE(frame.get_format() == EXPECTED_FORMAT);

    auto frame_data = frame.get_data();
    for (size_t i = 0; i < EXPECTED_DLC; ++i) {
        REQUIRE(frame_data[i] == EXPECTED_DATA[i]);
    }

    // Serialize and verify checksum
    auto buffer = frame.serialize();
    REQUIRE(buffer[19] == EXPECTED_CHECKSUM);

    // Byte-by-byte verification
    for (size_t i = 0; i < 20; ++i) {
        INFO("Byte index: " << i);
        REQUIRE(buffer[i] == KNOWN_FRAME_DUMP[i]);
    }
}

TEST_CASE("FixedFrame - Serialize-deserialize round-trip preserves data",
    "[roundtrip][serialize]") {

    FixedFrame original(Format::DATA_FIXED, CANVersion::EXT_FIXED, 0x1BCDEF12);
    original.set_format(Format::DATA_FIXED);
    std::array<std::uint8_t, 8> data = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    original.set_data(span<const std::uint8_t>(data.data(), 8));

    // Serialize
    const auto serialized = original.serialize();

    // Deserialize into new frame
    FixedFrame deserialized;
    auto result = deserialized.deserialize(serialized);

    REQUIRE(result.ok());

    // Verify all fields match
    REQUIRE(deserialized.get_can_id() == original.get_can_id());
    REQUIRE(deserialized.get_dlc() == original.get_dlc());
    REQUIRE(deserialized.get_format() == original.get_format());

    // Verify checksums match
    auto deserialized_buffer = deserialized.serialize();
    REQUIRE(deserialized_buffer[19] == serialized[19]);
}

// ============================================================================
// SIZE VERIFICATION
// ============================================================================

TEST_CASE("FixedFrame - Frame size is always 20 bytes", "[size]") {
    FixedFrame frame;
    REQUIRE(frame.size() == 20);

    // Size remains constant after modifications (use valid standard ID)
    frame.set_id(0x123);
    std::array<std::uint8_t, 8> data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    frame.set_data(span<const std::uint8_t>(data.data(), 8));
    REQUIRE(frame.size() == 20);
}

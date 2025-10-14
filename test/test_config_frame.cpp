/**
 * @file test_config_frame.cpp
 * @brief Focused unit tests for ConfigFrame class using Catch2
 *
 * Test Strategy:
 * 1. Individual getter/setter pairs
 * 2. Constructor parameter validation
 * 3. Checksum integrity via serialization
 * 4. Round-trip serialization/deserialization
 *
 * @note Build: cd build && cmake .. && cmake --build .
 * @note Run: ./test_config_frame
 */

#include <catch2/catch_test_macros.hpp>
#include "../include/frame/config_frame.hpp"
#include "../include/interface/serialization_helpers.hpp"
#include <array>

using namespace waveshare;

// ============================================================================
// TEST FIXTURE - Minimal shared setup
// ============================================================================
struct ConfigFrameFixture {
    // Predefined frame dump from protocol documentation
    // AA 55 12 01 01 00 00 07 ff 00 00 07 ff 00 00 00 00 00 00 20
    static constexpr std::array<std::uint8_t, 20> KNOWN_FRAME_DUMP = {
        0xAA,  // [0]  START
        0x55,  // [1]  HEADER (reserved, always 0x55)
        0x12,  // [2]  TYPE (CONF_VARIABLE)
        0x01,  // [3]  BAUD (BAUD_1M)
        0x01,  // [4]  CAN_VERS (STD_FIXED)
        0x00,  // [5]  FILTER[0]
        0x00,  // [6]  FILTER[1]
        0x07,  // [7]  FILTER[2]
        0xFF,  // [8]  FILTER[3]
        0x00,  // [9]  MASK[0]
        0x00,  // [10] MASK[1]
        0x07,  // [11] MASK[2]
        0xFF,  // [12] MASK[3]
        0x00,  // [13] MODE (NORMAL = 0x00)
        0x00,  // [14] AUTO_RTX (AUTO = 0x00)
        0x00,  // [15] RESERVED
        0x00,  // [16] RESERVED
        0x00,  // [17] RESERVED
        0x00,  // [18] RESERVED
        0x20   // [19] CHECKSUM
    };

    // Expected values from the dump above
    static constexpr CANBaud EXPECTED_BAUD = CANBaud::BAUD_1M;
    static constexpr uint32_t EXPECTED_FILTER = 0x000007FF;
    static constexpr uint32_t EXPECTED_MASK = 0x000007FF;
    static constexpr RTX EXPECTED_RTX = RTX::AUTO;
    static constexpr CANMode EXPECTED_MODE = CANMode::NORMAL;
    static constexpr std::uint8_t EXPECTED_CHECKSUM = 0x20;
};

// ============================================================================
// CONSTRUCTOR TESTS
// ============================================================================

TEST_CASE("ConfigFrame - Default constructor initializes with protocol defaults",
    "[constructor]") {
    ConfigFrame frame;

    // Verify defaults
    REQUIRE(frame.get_baud_rate() == CANBaud::BAUD_1M);
    REQUIRE(frame.get_can_mode() == CANMode::NORMAL);
    REQUIRE(frame.get_auto_rtx() == RTX::AUTO);
    REQUIRE(frame.get_filter() == 0x00000000);
    REQUIRE(frame.get_mask() == 0x00000000);
    REQUIRE(frame.get_type() == Type::CONF_FIXED);
}

TEST_CASE("ConfigFrame - Parameterized constructor sets all fields correctly",
    "[constructor]") {
    ConfigFrame frame(
        Type::CONF_VARIABLE,
        CANBaud::BAUD_500K,
        CANMode::LOOPBACK,
        RTX::OFF,
        0x12345678,  // filter (uint32_t)
        0x0007FFFF,  // mask (uint32_t)
        CANVersion::STD_FIXED
    );

    REQUIRE(frame.get_type() == Type::CONF_VARIABLE);
    REQUIRE(frame.get_filter() == 0x12345678);
    REQUIRE(frame.get_mask() == 0x0007FFFF);
    REQUIRE(frame.get_auto_rtx() == RTX::OFF);
    REQUIRE(frame.get_baud_rate() == CANBaud::BAUD_500K);
    REQUIRE(frame.get_can_mode() == CANMode::LOOPBACK);
}

// ============================================================================
// GETTER/SETTER TESTS - Individual field operations
// ============================================================================

TEST_CASE("ConfigFrame - Baud rate getter/setter round-trip", "[getter][setter][baud]") {
    ConfigFrame frame;

    // Test each baud rate enum value
    const std::array<CANBaud, 10> baud_rates = {
        CANBaud::BAUD_5K, CANBaud::BAUD_10K, CANBaud::BAUD_20K,
        CANBaud::BAUD_50K, CANBaud::BAUD_100K, CANBaud::BAUD_125K,
        CANBaud::BAUD_250K, CANBaud::BAUD_500K, CANBaud::BAUD_800K,
        CANBaud::BAUD_1M
    };

    for (const auto& baud : baud_rates) {
        frame.set_baud_rate(baud);
        REQUIRE(frame.get_baud_rate() == baud);
    }
}

TEST_CASE("ConfigFrame - CAN mode getter/setter round-trip", "[getter][setter][mode]") {
    ConfigFrame frame;

    const std::array<CANMode, 4> modes = {
        CANMode::NORMAL, CANMode::LOOPBACK,
        CANMode::SILENT, CANMode::LOOPBACK_SILENT
    };

    for (const auto& mode : modes) {
        frame.set_can_mode(mode);
        REQUIRE(frame.get_can_mode() == mode);
    }
}

TEST_CASE("ConfigFrame - Auto RTX getter/setter round-trip", "[getter][setter][rtx]") {
    ConfigFrame frame;

    frame.set_auto_rtx(RTX::AUTO);
    REQUIRE(frame.get_auto_rtx() == RTX::AUTO);

    frame.set_auto_rtx(RTX::OFF);
    REQUIRE(frame.get_auto_rtx() == RTX::OFF);
}

TEST_CASE("ConfigFrame - Filter getter/setter round-trip", "[getter][setter][filter]") {
    ConfigFrame frame;

    SECTION("Standard CAN ID boundaries") {
        frame.set_filter(0x000);
        REQUIRE(frame.get_filter() == 0x000);

        frame.set_filter(0x7FF);  // Max 11-bit
        REQUIRE(frame.get_filter() == 0x7FF);
    }

    SECTION("Extended CAN ID boundaries") {
        frame.set_filter(0x00000000);
        REQUIRE(frame.get_filter() == 0x00000000);

        frame.set_filter(0x1FFFFFFF);  // Max 29-bit
        REQUIRE(frame.get_filter() == 0x1FFFFFFF);
    }

    SECTION("Arbitrary values with endianness preservation") {
        frame.set_filter(0x12345678);
        REQUIRE(frame.get_filter() == 0x12345678);

        frame.set_filter(0xAABBCCDD);
        REQUIRE(frame.get_filter() == 0xAABBCCDD);
    }
}

TEST_CASE("ConfigFrame - Mask getter/setter round-trip", "[getter][setter][mask]") {
    ConfigFrame frame;

    frame.set_mask(0x00000000);
    REQUIRE(frame.get_mask() == 0x00000000);

    frame.set_mask(0x7FF);        // Standard mask
    REQUIRE(frame.get_mask() == 0x7FF);

    frame.set_mask(0x1FFFFFFF);   // Extended mask
    REQUIRE(frame.get_mask() == 0x1FFFFFFF);
}

TEST_CASE("ConfigFrame - Type getter/setter round-trip", "[getter][setter][type]") {
    ConfigFrame frame;

    frame.set_type(Type::CONF_VARIABLE);
    REQUIRE(frame.get_type() == Type::CONF_VARIABLE);

    frame.set_type(Type::CONF_FIXED);
    REQUIRE(frame.get_type() == Type::CONF_FIXED);
}

TEST_CASE("ConfigFrame - Field independence (setters don't interfere)",
    "[getter][setter][independence]") {
    ConfigFrame frame;

    // Set all fields to non-default values
    frame.set_baud_rate(CANBaud::BAUD_500K);
    frame.set_can_mode(CANMode::LOOPBACK);
    frame.set_auto_rtx(RTX::OFF);
    frame.set_filter(0x12345678);
    frame.set_mask(0x1FFFFFFF);
    frame.set_type(Type::CONF_FIXED);

    // Verify all fields retain their values
    REQUIRE(frame.get_baud_rate() == CANBaud::BAUD_500K);
    REQUIRE(frame.get_can_mode() == CANMode::LOOPBACK);
    REQUIRE(frame.get_auto_rtx() == RTX::OFF);
    REQUIRE(frame.get_filter() == 0x12345678);
    REQUIRE(frame.get_mask() == 0x1FFFFFFF);
    REQUIRE(frame.get_type() == Type::CONF_FIXED);

    // Change one field
    frame.set_baud_rate(CANBaud::BAUD_250K);

    // Verify others unchanged
    REQUIRE(frame.get_can_mode() == CANMode::LOOPBACK);
    REQUIRE(frame.get_auto_rtx() == RTX::OFF);
    REQUIRE(frame.get_filter() == 0x12345678);
    REQUIRE(frame.get_mask() == 0x1FFFFFFF);
}

// ============================================================================
// CHECKSUM TESTS - Integrity via serialization
// ============================================================================

TEST_CASE("ConfigFrame - Checksum calculation matches standalone method",
    "[checksum][integrity]") {
    ConfigFrame frame;

    // Set known values
    frame.set_baud_rate(CANBaud::BAUD_500K);
    frame.set_filter(0x123);
    frame.set_mask(0x7FF);

    // Serialize frame (checksum computed automatically)
    auto serialized = frame.serialize();

    // Verify checksum using ChecksumHelper
    REQUIRE(ChecksumHelper::validate(serialized, 19, 2, 18) == true);
}

TEST_CASE("ConfigFrame - Checksum updates after field modifications",
    "[checksum][dirty]") {
    ConfigFrame frame;

    // Initial serialization
    auto serialized1 = frame.serialize();
    std::uint8_t checksum1 = serialized1[19];

    // Modify a field
    frame.set_baud_rate(CANBaud::BAUD_250K);

    // Serialize again
    auto serialized2 = frame.serialize();
    std::uint8_t checksum2 = serialized2[19];

    // Checksums should differ
    REQUIRE(checksum1 != checksum2);
}

TEST_CASE("ConfigFrame - Checksum verification after serialization", "[checksum][verify]") {
    ConfigFrame frame;

    frame.set_baud_rate(CANBaud::BAUD_500K);
    frame.set_filter(0x123);

    auto serialized = frame.serialize();

    // Checksum should be valid
    REQUIRE(ChecksumHelper::validate(serialized, 19, 2, 18) == true);
}

TEST_CASE("ConfigFrame - Static checksum methods work on external data",
    "[checksum][static]") {
    // Create a valid frame dump manually
    std::array<std::uint8_t, 20> data = {
        0xAA, 0x55, 0x02, 0x01,  // START, HEADER, TYPE, BAUD
        0x01, 0x00, 0x00, 0x00,  // CAN_VERS, FILTER[0-2]
        0x00, 0x00, 0x00, 0x00,  // FILTER[3], MASK[0-3]
        0x00, 0x00, 0x00, 0x00,  // MODE, AUTO_RTX, RESERVED[0-1]
        0x00, 0x00, 0x00, 0x00   // RESERVED[2-3], Checksum placeholder
    };

    // Compute and write checksum
    ChecksumHelper::write(data, 19, 2, 18);

    // Verify it validates
    REQUIRE(ChecksumHelper::validate(data, 19, 2, 18) == true);
}

// ============================================================================
// ROUND-TRIP TESTS - Serialization/Deserialization
// ============================================================================

TEST_CASE_METHOD(ConfigFrameFixture,
    "ConfigFrame - Round-trip from known frame dump", "[roundtrip][integration]") {

    ConfigFrame frame(
        Type::CONF_VARIABLE,
        CANBaud::BAUD_1M,
        CANMode::NORMAL,
        RTX::AUTO,
        EXPECTED_FILTER,  // 0x000007FF
        EXPECTED_MASK,    // 0x000007FF
        CANVersion::STD_FIXED
    );

    // Serialize frame
    auto serialized = frame.serialize();

    // Field-by-field verification
    REQUIRE(frame.get_baud_rate() == EXPECTED_BAUD);
    REQUIRE(frame.get_filter() == EXPECTED_FILTER);
    REQUIRE(frame.get_mask() == EXPECTED_MASK);
    REQUIRE(frame.get_auto_rtx() == EXPECTED_RTX);
    REQUIRE(frame.get_can_mode() == EXPECTED_MODE);

    // Verify checksum matches expected
    REQUIRE(serialized[19] == EXPECTED_CHECKSUM);

    // Verify entire frame matches dump byte-by-byte
    REQUIRE(serialized.size() == KNOWN_FRAME_DUMP.size());
    for (size_t i = 0; i < serialized.size(); ++i) {
        INFO("Byte index: " << i);
        REQUIRE(serialized[i] == KNOWN_FRAME_DUMP[i]);
    }
}

TEST_CASE("ConfigFrame - Serialize-deserialize round-trip preserves data",
    "[roundtrip][serialize]") {

    // Create a frame with specific values
    ConfigFrame original;
    original.set_CAN_version(CANVersion::EXT_FIXED);
    original.set_baud_rate(CANBaud::BAUD_500K);
    original.set_can_mode(CANMode::LOOPBACK);
    original.set_auto_rtx(RTX::OFF);
    original.set_filter(0xABCDEF12);
    original.set_mask(0x1FFFFFFF);

    // Serialize
    auto serialized = original.serialize();
    REQUIRE(serialized.size() == 20);
    REQUIRE(serialized[0] == 0xAA);  // START byte

    // Deserialize into new frame
    ConfigFrame deserialized;
    REQUIRE_NOTHROW(deserialized.deserialize(span<const std::uint8_t>(serialized.data(),
        serialized.size())));

    // Verify all fields match
    REQUIRE(deserialized.get_CAN_version() == original.get_CAN_version());
    REQUIRE(deserialized.get_baud_rate() == original.get_baud_rate());
    REQUIRE(deserialized.get_can_mode() == original.get_can_mode());
    REQUIRE(deserialized.get_auto_rtx() == original.get_auto_rtx());
    REQUIRE(deserialized.get_filter() == original.get_filter());
    REQUIRE(deserialized.get_mask() == original.get_mask());

    // Verify serialization matches
    auto reserialized = deserialized.serialize();
    for (size_t i = 0; i < 20; ++i) {
        REQUIRE(reserialized[i] == serialized[i]);
    }
}

// ============================================================================
// SIZE VERIFICATION
// ============================================================================

TEST_CASE("ConfigFrame - Frame size is always 20 bytes", "[size]") {
    ConfigFrame frame;
    REQUIRE(frame.serialized_size() == 20);

    // Size remains constant after modifications
    frame.set_baud_rate(CANBaud::BAUD_250K);
    frame.set_filter(0x12345678);
    REQUIRE(frame.serialized_size() == 20);
}
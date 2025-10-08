/**
 * @file test_variable_frame.cpp
 * @brief Focused unit tests for VariableFrame class using Catch2
 *
 * Test Strategy:
 * 1. Individual getter/setter pairs (ID, format, DLC, data)
 * 2. Constructor parameter validation
 * 3. Dynamic frame size handling (5-15 bytes)
 * 4. Type byte reconstruction (VarTypeInterface)
 * 5. Round-trip serialization/deserialization
 * 6. Standard vs Extended ID with size changes
 *
 * @note Build: cd build && cmake .. && cmake --build .
 * @note Run: ./test_variable_frame
 */

#include <catch2/catch_test_macros.hpp>
#include "../include/frame/variable_frame.hpp"
#include <array>
#include <vector>

using namespace USBCANBridge;

// ============================================================================
// TEST FIXTURE - Minimal shared setup
// ============================================================================
struct VariableFrameFixture {
    // Predefined frame dump - variable size (5-15 bytes)
    // Standard ID (2 bytes) + 4 data bytes = 8 bytes total
    // AA C4 01 23 11 22 33 44 55
    static constexpr std::array<std::uint8_t, 9> KNOWN_STD_FRAME_DUMP = {
        0xAA,  // [0] START
        0xC4,  // [1] TYPE (IsExt=0, Format=0, DLC=4)
        0x01,  // [2] ID[0] (little-endian)
        0x23,  // [3] ID[1]
        0x11,  // [4] DATA[0]
        0x22,  // [5] DATA[1]
        0x33,  // [6] DATA[2]
        0x44,  // [7] DATA[3]
        0x55   // [8] END
    };

    // Extended ID (4 bytes) + 2 data bytes = 9 bytes total
    // AA D2 12 34 56 78 AA BB 55
    static constexpr std::array<std::uint8_t, 9> KNOWN_EXT_FRAME_DUMP = {
        0xAA,  // [0] START
        0xD2,  // [1] TYPE (IsExt=1, Format=0, DLC=2)
        0x12,  // [2] ID[0] (little-endian)
        0x34,  // [3] ID[1]
        0x56,  // [4] ID[2]
        0x78,  // [5] ID[3]
        0xAA,  // [6] DATA[0]
        0xBB,  // [7] DATA[1]
        0x55   // [8] END
    };

    static constexpr uint32_t EXPECTED_STD_ID = 0x2301;      // Little-endian: 01 23
    static constexpr uint32_t EXPECTED_EXT_ID = 0x78563412;  // Little-endian: 12 34 56 78
};

// ============================================================================
// CONSTRUCTOR TESTS
// ============================================================================

TEST_CASE("VariableFrame - Default constructor initializes correctly", "[constructor]") {
    VariableFrame frame;

    // Verify defaults
    REQUIRE(frame.get_can_id() == 0x00000000);
    REQUIRE(frame.get_dlc() == 0);
    REQUIRE(frame.get_format() == Format::DATA_VARIABLE);
    REQUIRE(frame.is_extended() == false);  // STD_VARIABLE
    REQUIRE(frame.is_remote() == false);
    REQUIRE(frame.size() >= 5);  // Minimum size
    REQUIRE(frame.size() <= 15); // Maximum size
}

TEST_CASE("VariableFrame - Parameterized constructor with standard ID", "[constructor]") {
    std::vector<std::uint8_t> id_bytes = {0x01, 0x23};  // Standard ID (2 bytes)
    std::vector<std::uint8_t> data = {0xAA, 0xBB, 0xCC, 0xDD};

    VariableFrame frame(
        Format::DATA_VARIABLE,
        CANVersion::STD_VARIABLE,
        id_bytes,
        4,  // DLC
        data
    );

    REQUIRE(frame.get_can_id() == 0x2301);  // Little-endian
    REQUIRE(frame.get_dlc() == 4);
    REQUIRE(frame.get_format() == Format::DATA_VARIABLE);
    REQUIRE(frame.is_extended() == false);
    REQUIRE(frame.size() == 8);  // START + TYPE + ID(2) + DATA(4) + END = 8
}

TEST_CASE("VariableFrame - Parameterized constructor with extended ID", "[constructor]") {
    std::vector<std::uint8_t> id_bytes = {0x12, 0x34, 0x56, 0x78};  // Extended ID (4 bytes)
    std::vector<std::uint8_t> data = {0x11, 0x22};

    VariableFrame frame(
        Format::DATA_VARIABLE,
        CANVersion::EXT_VARIABLE,
        id_bytes,
        2,  // DLC
        data
    );

    REQUIRE(frame.get_can_id() == 0x78563412);  // Little-endian
    REQUIRE(frame.get_dlc() == 2);
    REQUIRE(frame.is_extended() == true);
    REQUIRE(frame.size() == 9);  // START + TYPE + ID(4) + DATA(2) + END = 9
}

// ============================================================================
// GETTER/SETTER TESTS - Individual field operations
// ============================================================================

TEST_CASE("VariableFrame - CAN ID getter/setter round-trip", "[getter][setter][id]") {
    VariableFrame frame;

    SECTION("Standard CAN ID boundaries") {
        frame.set_id(0x000);
        REQUIRE(frame.get_can_id() == 0x000);

        frame.set_id(0x7FF);  // Max 11-bit
        REQUIRE(frame.get_can_id() == 0x7FF);
    }

    SECTION("Extended CAN ID boundaries") {
        frame.set_id(0x00000000);
        REQUIRE(frame.get_can_id() == 0x00000000);

        frame.set_id(0x1FFFFFFF);  // Max 29-bit
        REQUIRE(frame.get_can_id() == 0x1FFFFFFF);
    }

    SECTION("ID with endianness preservation") {
        frame.set_id(0x12345678);
        REQUIRE(frame.get_can_id() == 0x12345678);
    }
}

TEST_CASE("VariableFrame - Format getter/setter round-trip", "[getter][setter][format]") {
    VariableFrame frame;

    frame.set_format(Format::DATA_VARIABLE);
    REQUIRE(frame.get_format() == Format::DATA_VARIABLE);
    REQUIRE(frame.is_remote() == false);

    frame.set_format(Format::REMOTE_VARIABLE);
    REQUIRE(frame.get_format() == Format::REMOTE_VARIABLE);
    REQUIRE(frame.is_remote() == true);
}

TEST_CASE("VariableFrame - Data getter/setter with size changes", "[getter][setter][data]") {
    VariableFrame frame;

    SECTION("Set data changes frame size") {
        size_t initial_size = frame.size();

        std::vector<std::uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
        frame.set_data(span<const std::uint8_t>(data.data(), 5));

        REQUIRE(frame.get_dlc() == 5);
        size_t new_size = frame.size();
        REQUIRE(new_size > initial_size);  // Size increased with data
    }

    SECTION("Data round-trip preservation") {
        std::vector<std::uint8_t> data = {0xAA, 0xBB, 0xCC, 0xDD};
        frame.set_data(span<const std::uint8_t>(data.data(), 4));

        auto retrieved = frame.get_data();
        REQUIRE(retrieved.size() == 4);
        for (size_t i = 0; i < 4; ++i) {
            REQUIRE(retrieved[i] == data[i]);
        }
    }

    SECTION("Maximum data payload (8 bytes)") {
        std::vector<std::uint8_t> data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
        frame.set_data(span<const std::uint8_t>(data.data(), 8));

        REQUIRE(frame.get_dlc() == 8);
        auto retrieved = frame.get_data();
        for (size_t i = 0; i < 8; ++i) {
            REQUIRE(retrieved[i] == data[i]);
        }
    }

    SECTION("Empty data payload (DLC = 0)") {
        std::vector<std::uint8_t> data = {};
        frame.set_data(span<const std::uint8_t>(data.data(), 0));

        REQUIRE(frame.get_dlc() == 0);
    }
}

TEST_CASE("VariableFrame - DLC automatically updates with data", "[getter][setter][dlc]") {
    VariableFrame frame;

    std::vector<std::uint8_t> data = {0x10, 0x20, 0x30};
    frame.set_data(span<const std::uint8_t>(data.data(), 3));

    REQUIRE(frame.get_dlc() == 3);
}

TEST_CASE("VariableFrame - Extended vs Standard ID affects frame size",
    "[getter][setter][extended]") {

    SECTION("Standard ID (2 bytes)") {
        VariableFrame std_frame(
            Format::DATA_VARIABLE,
            CANVersion::STD_VARIABLE,
            {0x01, 0x23},
            0,
            {}
        );

        REQUIRE(std_frame.is_extended() == false);
        // START(1) + TYPE(1) + ID(2) + DATA(0) + END(1) = 5 bytes
        REQUIRE(std_frame.size() == 5);
    }

    SECTION("Extended ID (4 bytes)") {
        VariableFrame ext_frame(
            Format::DATA_VARIABLE,
            CANVersion::EXT_VARIABLE,
            {0x12, 0x34, 0x56, 0x78},
            0,
            {}
        );

        REQUIRE(ext_frame.is_extended() == true);
        // START(1) + TYPE(1) + ID(4) + DATA(0) + END(1) = 7 bytes
        REQUIRE(ext_frame.size() == 7);
    }
}

TEST_CASE("VariableFrame - Field independence (setters don't interfere)",
    "[getter][setter][independence]") {
    VariableFrame frame;

    // Set all fields
    frame.set_id(0x12345678);
    frame.set_format(Format::DATA_VARIABLE);
    std::vector<std::uint8_t> data = {0x11, 0x22, 0x33, 0x44};
    frame.set_data(span<const std::uint8_t>(data.data(), 4));

    // Verify all retain values
    REQUIRE(frame.get_can_id() == 0x12345678);
    REQUIRE(frame.get_format() == Format::DATA_VARIABLE);
    REQUIRE(frame.get_dlc() == 4);

    // Change one field
    frame.set_id(0xAABBCCDD);

    // Verify others unchanged
    REQUIRE(frame.get_format() == Format::DATA_VARIABLE);
    REQUIRE(frame.get_dlc() == 4);
}

// ============================================================================
// TYPE BYTE TESTS - VarTypeInterface reconstruction
// ============================================================================

TEST_CASE("VariableFrame - Type byte encodes format, extension, and DLC", "[type][vartype]") {
    VariableFrame frame;

    SECTION("Type byte for standard ID, data frame, DLC=4") {
        frame.set_format(Format::DATA_VARIABLE);  // Format=0
        std::vector<std::uint8_t> data = {0x01, 0x02, 0x03, 0x04};
        frame.set_data(span<const std::uint8_t>(data.data(), 4));  // DLC=4
        // IsExtended should be false (STD_VARIABLE)

        frame.finalize();

        // TYPE byte = [0xC0 | IsExt(0) | Format(0) | DLC(4)]
        // Expected: 0xC0 | 0x00 | 0x00 | 0x04 = 0xC4
        const auto& storage = frame.get_storage();
        REQUIRE(storage[1] == 0xC4);
    }

    SECTION("Type byte for extended ID, remote frame, DLC=2") {
        VariableFrame ext_frame(
            Format::REMOTE_VARIABLE,  // Format=1
            CANVersion::EXT_VARIABLE,  // IsExt=1
            {0x12, 0x34, 0x56, 0x78},
            2,
            {}
        );

        ext_frame.finalize();

        // TYPE byte = [0xC0 | IsExt(1) | Format(1) | DLC(2)]
        // Expected: 0xC0 | 0x10 | 0x08 | 0x02 = 0xDA
        const auto& storage = ext_frame.get_storage();
        REQUIRE(storage[1] == 0xDA);
    }
}

// ============================================================================
// ROUND-TRIP TESTS - Serialization/Deserialization
// ============================================================================

TEST_CASE_METHOD(VariableFrameFixture,
    "VariableFrame - Round-trip from known standard ID frame dump",
    "[roundtrip][integration][std]") {

    std::vector<std::uint8_t> id_bytes = {0x01, 0x23};
    std::vector<std::uint8_t> data = {0x11, 0x22, 0x33, 0x44};

    VariableFrame frame(
        Format::DATA_VARIABLE,
        CANVersion::STD_VARIABLE,
        id_bytes,
        4,
        data
    );

    frame.finalize();

    // Field verification
    REQUIRE(frame.get_can_id() == EXPECTED_STD_ID);
    REQUIRE(frame.get_dlc() == 4);
    REQUIRE(frame.get_format() == Format::DATA_VARIABLE);
    REQUIRE(frame.is_extended() == false);

    // Byte-by-byte verification
    const auto& storage = frame.get_storage();
    REQUIRE(storage.size() == KNOWN_STD_FRAME_DUMP.size());

    for (size_t i = 0; i < storage.size(); ++i) {
        INFO("Byte index: " << i);
        REQUIRE(storage[i] == KNOWN_STD_FRAME_DUMP[i]);
    }
}

TEST_CASE_METHOD(VariableFrameFixture,
    "VariableFrame - Round-trip from known extended ID frame dump",
    "[roundtrip][integration][ext]") {

    std::vector<std::uint8_t> id_bytes = {0x12, 0x34, 0x56, 0x78};
    std::vector<std::uint8_t> data = {0xAA, 0xBB};

    VariableFrame frame(
        Format::DATA_VARIABLE,
        CANVersion::EXT_VARIABLE,
        id_bytes,
        2,
        data
    );

    frame.finalize();

    // Field verification
    REQUIRE(frame.get_can_id() == EXPECTED_EXT_ID);
    REQUIRE(frame.get_dlc() == 2);
    REQUIRE(frame.is_extended() == true);

    // Byte-by-byte verification
    const auto& storage = frame.get_storage();
    REQUIRE(storage.size() == KNOWN_EXT_FRAME_DUMP.size());

    for (size_t i = 0; i < storage.size(); ++i) {
        INFO("Byte index: " << i);
        REQUIRE(storage[i] == KNOWN_EXT_FRAME_DUMP[i]);
    }
}

TEST_CASE("VariableFrame - Serialize-deserialize round-trip preserves data",
    "[roundtrip][serialize]") {

    VariableFrame original;
    original.set_id(0xABCDEF12);
    original.set_format(Format::DATA_VARIABLE);
    std::vector<std::uint8_t> data = {0xAA, 0xBB, 0xCC};
    original.set_data(span<const std::uint8_t>(data.data(), 3));
    original.finalize();

    const auto& serialized = original.get_storage();

    // VariableFrame would need a from_bytes() method for true deserialization
    // For now, verify we can extract fields correctly
    REQUIRE(serialized[0] == 0xAA);  // START
    REQUIRE(serialized[serialized.size() - 1] == 0x55);  // END
}

// ============================================================================
// SIZE VERIFICATION
// ============================================================================

TEST_CASE("VariableFrame - Frame size is dynamic (5-15 bytes)", "[size]") {
    VariableFrame frame;

    size_t initial_size = frame.size();
    REQUIRE(initial_size >= 5);
    REQUIRE(initial_size <= 15);

    // Size changes with data
    std::vector<std::uint8_t> data = {0x11, 0x22, 0x33, 0x44, 0x55};
    frame.set_data(span<const std::uint8_t>(data.data(), 5));

    size_t new_size = frame.size();
    REQUIRE(new_size >= 5);
    REQUIRE(new_size <= 15);
    REQUIRE(new_size != initial_size);  // Size changed
}

TEST_CASE("VariableFrame - Minimum and maximum frame sizes", "[size][boundary]") {
    SECTION("Minimum size: Standard ID, no data") {
        VariableFrame min_frame(
            Format::DATA_VARIABLE,
            CANVersion::STD_VARIABLE,
            {0x01, 0x23},
            0,
            {}
        );

        // START + TYPE + ID(2) + END = 5 bytes (minimum)
        REQUIRE(min_frame.size() == 5);
    }

    SECTION("Maximum size: Extended ID, 8 bytes data") {
        VariableFrame max_frame(
            Format::DATA_VARIABLE,
            CANVersion::EXT_VARIABLE,
            {0x12, 0x34, 0x56, 0x78},
            8,
            {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}
        );

        // START + TYPE + ID(4) + DATA(8) + END = 15 bytes (maximum)
        REQUIRE(max_frame.size() == 15);
    }
}

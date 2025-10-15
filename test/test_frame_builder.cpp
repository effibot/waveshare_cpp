/**
 * @file test_frame_builder.cpp
 * @brief Unit tests for FrameBuilder pattern (v3.0)
 *
 * Test Strategy:
 * 1. Verify builder methods set correct state
 * 2. Compare builder-constructed frames with constructor-created frames
 * 3. Test validation (ID ranges, data size)
 * 4. Test convenience features (move semantics, initializer lists)
 * 5. Test defaults and error handling
 *
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @date 2025-10-10
 */

#include <catch2/catch_test_macros.hpp>
#include "../include/pattern/frame_builder.hpp"
#include "../include/frame/fixed_frame.hpp"
#include "../include/frame/variable_frame.hpp"
#include "../include/frame/config_frame.hpp"
#include <vector>

using namespace waveshare;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * @brief Compare two frames by serialization (byte-for-byte equality)
 */
template<typename Frame>
bool frames_equal(const Frame& a, const Frame& b) {
    auto a_bytes = a.serialize();
    auto b_bytes = b.serialize();
    return a_bytes == b_bytes;
}

// ============================================================================
// FIXED FRAME BUILDER TESTS
// ============================================================================

TEST_CASE("FixedFrame - Builder produces identical frame to constructor", "[builder][fixed]") {
    SECTION("Standard ID with data") {
        std::vector<std::uint8_t> data = {0x11, 0x22, 0x33, 0x44};

        // Build with constructor
        FixedFrame ctor_frame(
            Format::DATA_FIXED,
            CANVersion::STD_FIXED,
            0x123,
            span<const std::uint8_t>(data.data(), data.size())
        );

        // Build with builder
        auto builder_frame = make_fixed_frame()
            .with_can_version(CANVersion::STD_FIXED)
            .with_format(Format::DATA_FIXED)
            .with_id(0x123)
            .with_data(data)
            .build();

        REQUIRE(frames_equal(ctor_frame, builder_frame));
        REQUIRE(builder_frame.get_can_id() == 0x123);
        REQUIRE(builder_frame.get_dlc() == 4);
    }

    SECTION("Extended ID with data") {
        std::vector<std::uint8_t> data = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

        FixedFrame ctor_frame(
            Format::DATA_FIXED,
            CANVersion::EXT_FIXED,
            0x1ABCDEF0,
            span<const std::uint8_t>(data.data(), data.size())
        );

        auto builder_frame = make_fixed_frame()
            .with_can_version(CANVersion::EXT_FIXED)
            .with_format(Format::DATA_FIXED)
            .with_id(0x1ABCDEF0)
            .with_data(data)
            .build();

        REQUIRE(frames_equal(ctor_frame, builder_frame));
        REQUIRE(builder_frame.get_can_id() == 0x1ABCDEF0);
        REQUIRE(builder_frame.is_extended() == true);
    }

    SECTION("Remote frame (no data)") {
        FixedFrame ctor_frame(
            Format::REMOTE_FIXED,
            CANVersion::STD_FIXED,
            0x200,
            span<const std::uint8_t>()
        );

        auto builder_frame = make_fixed_frame()
            .with_can_version(CANVersion::STD_FIXED)
            .with_format(Format::REMOTE_FIXED)
            .with_id(0x200)
            .build();

        REQUIRE(frames_equal(ctor_frame, builder_frame));
        REQUIRE(builder_frame.is_remote() == true);
        REQUIRE(builder_frame.get_dlc() == 0);
    }
}

TEST_CASE("FixedFrame - Builder with initializer list", "[builder][fixed]") {
    auto builder_frame = make_fixed_frame()
        .with_can_version(CANVersion::STD_FIXED)
        .with_format(Format::DATA_FIXED)
        .with_id(0x456)
        .with_data({0x11, 0x22, 0x33})  // initializer list
        .build();

    std::vector<std::uint8_t> data = {0x11, 0x22, 0x33};
    FixedFrame ctor_frame(
        Format::DATA_FIXED,
        CANVersion::STD_FIXED,
        0x456,
        span<const std::uint8_t>(data.data(), data.size())
    );

    REQUIRE(frames_equal(ctor_frame, builder_frame));

    auto frame_data = builder_frame.get_data();
    REQUIRE(frame_data.size() == 3);
    REQUIRE(frame_data[0] == 0x11);
    REQUIRE(frame_data[1] == 0x22);
    REQUIRE(frame_data[2] == 0x33);
}

TEST_CASE("FixedFrame - Builder with move semantics", "[builder][fixed]") {
    std::vector<std::uint8_t> data = {0xAA, 0xBB, 0xCC, 0xDD};
    std::vector<std::uint8_t> data_copy = data;

    auto builder_frame = make_fixed_frame()
        .with_can_version(CANVersion::STD_FIXED)
        .with_format(Format::DATA_FIXED)
        .with_id(0x789)
        .with_data(std::move(data))  // move
        .build();

    FixedFrame ctor_frame(
        Format::DATA_FIXED,
        CANVersion::STD_FIXED,
        0x789,
        span<const std::uint8_t>(data_copy.data(), data_copy.size())
    );

    REQUIRE(frames_equal(ctor_frame, builder_frame));
}

TEST_CASE("FixedFrame - Builder automatic defaults", "[builder][fixed]") {
    // Only ID is required, everything else gets defaults
    auto builder_frame = make_fixed_frame()
        .with_id(0x100)
        .build();

    FixedFrame ctor_frame(
        Format::DATA_FIXED,
        CANVersion::STD_FIXED,
        0x100,
        span<const std::uint8_t>()
    );

    REQUIRE(frames_equal(ctor_frame, builder_frame));
    REQUIRE(builder_frame.get_CAN_version() == CANVersion::STD_FIXED);
    REQUIRE(builder_frame.get_format() == Format::DATA_FIXED);
}

TEST_CASE("FixedFrame - Builder ID validation", "[builder][fixed][validation]") {
    SECTION("Standard ID exceeds maximum") {
        REQUIRE_THROWS_AS(
            make_fixed_frame()
            .with_can_version(CANVersion::STD_FIXED)
            .with_id(0xFFFF)      // Max is 0x7FF for standard
            .build(),
            std::out_of_range  // Frame's set_id throws this
        );
    }

    SECTION("Extended ID within range") {
        REQUIRE_NOTHROW(
            make_fixed_frame()
            .with_can_version(CANVersion::EXT_FIXED)
            .with_id(0x1FFFFFFF)      // Max for extended
            .build()
        );
    }

    SECTION("Extended ID exceeds maximum") {
        REQUIRE_THROWS_AS(
            make_fixed_frame()
            .with_can_version(CANVersion::EXT_FIXED)
            .with_id(0xFFFFFFFF)      // Too large
            .build(),
            std::out_of_range  // Frame's set_id throws this
        );
    }
}

TEST_CASE("FixedFrame - Builder data size validation", "[builder][fixed][validation]") {
    REQUIRE_THROWS_AS(
        make_fixed_frame()
        .with_id(0x123)
        .with_data({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09})      // 9 bytes
        .build(),
        std::out_of_range  // Frame's set_data throws this
    );
}

// ============================================================================
// VARIABLE FRAME BUILDER TESTS
// ============================================================================

TEST_CASE("VariableFrame - Builder produces identical frame to constructor",
    "[builder][variable]") {
    SECTION("Standard ID with data") {
        std::vector<std::uint8_t> data = {0x11, 0x22, 0x33};

        VariableFrame ctor_frame(
            Format::DATA_VARIABLE,
            CANVersion::STD_VARIABLE,
            0x123,
            span<const std::uint8_t>(data.data(), data.size())
        );

        auto builder_frame = make_variable_frame()
            .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
            .with_id(0x123)
            .with_data(data)
            .build();

        REQUIRE(frames_equal(ctor_frame, builder_frame));
        REQUIRE(builder_frame.get_can_id() == 0x123);
        REQUIRE(builder_frame.get_dlc() == 3);
    }

    SECTION("Extended ID with data") {
        std::vector<std::uint8_t> data = {0xAA, 0xBB};

        VariableFrame ctor_frame(
            Format::DATA_VARIABLE,
            CANVersion::EXT_VARIABLE,
            0x1ABCDEF,
            span<const std::uint8_t>(data.data(), data.size())
        );

        auto builder_frame = make_variable_frame()
            .with_type(CANVersion::EXT_VARIABLE, Format::DATA_VARIABLE)
            .with_id(0x1ABCDEF)
            .with_data(data)
            .build();

        REQUIRE(frames_equal(ctor_frame, builder_frame));
        REQUIRE(builder_frame.is_extended() == true);
    }

    SECTION("Remote frame") {
        VariableFrame ctor_frame(
            Format::REMOTE_VARIABLE,
            CANVersion::STD_VARIABLE,
            0x200,
            span<const std::uint8_t>()
        );

        auto builder_frame = make_variable_frame()
            .with_type(CANVersion::STD_VARIABLE, Format::REMOTE_VARIABLE)
            .with_id(0x200)
            .build();

        REQUIRE(frames_equal(ctor_frame, builder_frame));
        REQUIRE(builder_frame.is_remote() == true);
    }
}

TEST_CASE("VariableFrame - Builder with initializer list", "[builder][variable]") {
    auto builder_frame = make_variable_frame()
        .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
        .with_id(0x456)
        .with_data({0x99, 0x88, 0x77, 0x66})
        .build();

    std::vector<std::uint8_t> data = {0x99, 0x88, 0x77, 0x66};
    VariableFrame ctor_frame(
        Format::DATA_VARIABLE,
        CANVersion::STD_VARIABLE,
        0x456,
        span<const std::uint8_t>(data.data(), data.size())
    );

    REQUIRE(frames_equal(ctor_frame, builder_frame));
}

TEST_CASE("VariableFrame - Builder ID validation", "[builder][variable][validation]") {
    SECTION("Standard ID exceeds maximum") {
        REQUIRE_THROWS_AS(
            make_variable_frame()
            .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
            .with_id(0x800)      // Max is 0x7FF
            .build(),
            std::out_of_range  // Frame's set_id throws this
        );
    }

    SECTION("Extended ID within range") {
        REQUIRE_NOTHROW(
            make_variable_frame()
            .with_type(CANVersion::EXT_VARIABLE, Format::DATA_VARIABLE)
            .with_id(0x1FFFFFFF)
            .build()
        );
    }
}

// ============================================================================
// CONFIG FRAME BUILDER TESTS
// ============================================================================

TEST_CASE("ConfigFrame - Builder produces identical frame to constructor", "[builder][config]") {
    SECTION("Complete configuration") {
        ConfigFrame ctor_frame(
            Type::CONF_VARIABLE,
            CANBaud::BAUD_500K,
            CANMode::NORMAL,
            RTX::AUTO,
            0x000007FF,
            0x000007FF,
            CANVersion::STD_FIXED
        );

        auto builder_frame = make_config_frame()
            .with_can_version(CANVersion::STD_FIXED)
            .with_type(Type::CONF_VARIABLE)
            .with_baud_rate(CANBaud::BAUD_500K)
            .with_mode(CANMode::NORMAL)
            .with_filter(0x000007FF)
            .with_mask(0x000007FF)
            .with_rtx(RTX::AUTO)
            .build();

        REQUIRE(frames_equal(ctor_frame, builder_frame));
        REQUIRE(builder_frame.get_baud_rate() == CANBaud::BAUD_500K);
        REQUIRE(builder_frame.get_can_mode() == CANMode::NORMAL);
        REQUIRE(builder_frame.get_filter() == 0x000007FF);
        REQUIRE(builder_frame.get_mask() == 0x000007FF);
    }

    SECTION("Minimal configuration with defaults") {
        ConfigFrame ctor_frame(
            Type::CONF_VARIABLE,
            CANBaud::BAUD_1M,
            CANMode::LOOPBACK,
            RTX::AUTO,
            0x00000000,
            0x00000000,
            CANVersion::STD_FIXED
        );

        auto builder_frame = make_config_frame()
            .with_can_version(CANVersion::STD_FIXED)
            .with_baud_rate(CANBaud::BAUD_1M)
            .with_mode(CANMode::LOOPBACK)
            .build();

        REQUIRE(frames_equal(ctor_frame, builder_frame));
    }
}

TEST_CASE("ConfigFrame - Builder with uint32_t filter/mask", "[builder][config]") {
    auto builder_frame = make_config_frame()
        .with_can_version(CANVersion::STD_FIXED)
        .with_baud_rate(CANBaud::BAUD_250K)
        .with_mode(CANMode::NORMAL)
        .with_filter(0x123)  // uint32_t
        .with_mask(0x456)    // uint32_t
        .build();

    ConfigFrame ctor_frame(
        Type::CONF_VARIABLE,
        CANBaud::BAUD_250K,
        CANMode::NORMAL,
        RTX::AUTO,
        0x123,
        0x456,
        CANVersion::STD_FIXED
    );

    REQUIRE(frames_equal(ctor_frame, builder_frame));
    REQUIRE(builder_frame.get_filter() == 0x123);
    REQUIRE(builder_frame.get_mask() == 0x456);
}

TEST_CASE("ConfigFrame - Builder with array filter/mask", "[builder][config]") {
    std::array<std::uint8_t, 4> filter = {0xFF, 0x07, 0x00, 0x00};  // 0x000007FF
    std::array<std::uint8_t, 4> mask = {0xFF, 0x07, 0x00, 0x00};

    auto builder_frame = make_config_frame()
        .with_can_version(CANVersion::STD_FIXED)
        .with_baud_rate(CANBaud::BAUD_125K)
        .with_mode(CANMode::SILENT)
        .with_filter(filter)
        .with_mask(mask)
        .build();

    // Convert arrays to uint32_t for constructor
    uint32_t filter_val = (static_cast<uint32_t>(filter[3]) << 24) |
        (static_cast<uint32_t>(filter[2]) << 16) |
        (static_cast<uint32_t>(filter[1]) << 8) |
        static_cast<uint32_t>(filter[0]);

    uint32_t mask_val = (static_cast<uint32_t>(mask[3]) << 24) |
        (static_cast<uint32_t>(mask[2]) << 16) |
        (static_cast<uint32_t>(mask[1]) << 8) |
        static_cast<uint32_t>(mask[0]);

    ConfigFrame ctor_frame(
        Type::CONF_VARIABLE,
        CANBaud::BAUD_125K,
        CANMode::SILENT,
        RTX::AUTO,
        filter_val,
        mask_val,
        CANVersion::STD_FIXED
    );

    REQUIRE(frames_equal(ctor_frame, builder_frame));
}

TEST_CASE("ConfigFrame - Builder requires baud_rate and mode", "[builder][config][validation]") {
    REQUIRE_THROWS_AS(
        make_config_frame()
        .with_can_version(CANVersion::STD_FIXED)
        // Missing baud_rate
        .with_mode(CANMode::NORMAL)
        .build(),
        std::runtime_error
    );

    REQUIRE_THROWS_AS(
        make_config_frame()
        .with_can_version(CANVersion::STD_FIXED)
        .with_baud_rate(CANBaud::BAUD_1M)
        // Missing mode
        .build(),
        std::runtime_error
    );
}

// ============================================================================
// GENERIC BUILDER TESTS
// ============================================================================

TEST_CASE("Generic make_frame<T> factory", "[builder][factory]") {
    auto fixed = make_frame<FixedFrame>()
        .with_id(0x123)
        .build();

    auto variable = make_frame<VariableFrame>()
        .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
        .with_id(0x456)
        .build();

    auto config = make_frame<ConfigFrame>()
        .with_baud_rate(CANBaud::BAUD_1M)
        .with_mode(CANMode::NORMAL)
        .build();

    REQUIRE(fixed.size() == 20);
    REQUIRE(variable.size() >= 5);
    REQUIRE(config.size() == 20);
}

TEST_CASE("Builder method chaining", "[builder][chaining]") {
    // All methods should return builder reference for chaining
    auto frame = make_fixed_frame()
        .with_can_version(CANVersion::STD_FIXED)
        .with_format(Format::DATA_FIXED)
        .with_id(0x123)
        .with_data({0x11, 0x22})
        .build();

    REQUIRE(frame.get_can_id() == 0x123);
}

TEST_CASE("Builder reuse with const &build()", "[builder][reuse]") {
    auto builder = make_fixed_frame()
        .with_id(0x123)
        .with_data({0x11, 0x22});

    // Build multiple frames from same builder state
    auto frame1 = builder.build();
    auto frame2 = builder.build();

    REQUIRE(frames_equal(frame1, frame2));
}

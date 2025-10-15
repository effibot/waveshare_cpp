/**
 * @file test_socketcan_helpers.cpp
 * @brief Unit tests for SocketCAN conversion helpers
 * @version 1.0
 * @date 2025-10-14
 */

#include <catch2/catch_test_macros.hpp>
#include <linux/can.h>

#include "../include/interface/socketcan_helpers.hpp"
#include "../include/frame/variable_frame.hpp"
#include "../include/exception/waveshare_exception.hpp"

using namespace waveshare;

TEST_CASE("SocketCANHelper::to_socketcan - Standard ID Data Frame", "[socketcan][conversion]") {
    // Create VariableFrame with standard ID
    auto frame = VariableFrame(
        Format::DATA_VARIABLE,
        CANVersion::STD_VARIABLE,
        0x123,  // 11-bit standard ID
        span<const std::uint8_t>({0x11, 0x22, 0x33})
    );

    // Convert to SocketCAN
    struct can_frame cf = SocketCANHelper::to_socketcan(frame);

    // Verify CAN ID (no EFF flag for standard)
    REQUIRE((cf.can_id & CAN_EFF_FLAG) == 0);
    REQUIRE((cf.can_id & CAN_SFF_MASK) == 0x123);
    REQUIRE((cf.can_id & CAN_RTR_FLAG) == 0);

    // Verify DLC and data
    REQUIRE(cf.can_dlc == 3);
    REQUIRE(cf.data[0] == 0x11);
    REQUIRE(cf.data[1] == 0x22);
    REQUIRE(cf.data[2] == 0x33);
}

TEST_CASE("SocketCANHelper::to_socketcan - Extended ID Data Frame", "[socketcan][conversion]") {
    // Create VariableFrame with extended ID
    auto frame = VariableFrame(
        Format::DATA_VARIABLE,
        CANVersion::EXT_VARIABLE,
        0x12345678,  // 29-bit extended ID
        span<const std::uint8_t>({0xAA, 0xBB, 0xCC, 0xDD})
    );

    // Convert to SocketCAN
    struct can_frame cf = SocketCANHelper::to_socketcan(frame);

    // Verify CAN ID with EFF flag
    REQUIRE((cf.can_id & CAN_EFF_FLAG) != 0);
    REQUIRE((cf.can_id & CAN_EFF_MASK) == 0x12345678);
    REQUIRE((cf.can_id & CAN_RTR_FLAG) == 0);

    // Verify DLC and data
    REQUIRE(cf.can_dlc == 4);
    REQUIRE(cf.data[0] == 0xAA);
    REQUIRE(cf.data[1] == 0xBB);
    REQUIRE(cf.data[2] == 0xCC);
    REQUIRE(cf.data[3] == 0xDD);
}

TEST_CASE("SocketCANHelper::to_socketcan - Remote Frame", "[socketcan][conversion]") {
    // Create remote VariableFrame
    auto frame = VariableFrame(
        Format::REMOTE_VARIABLE,
        CANVersion::STD_VARIABLE,
        0x456,
        span<const std::uint8_t>()  // No data for remote frame
    );

    // Convert to SocketCAN
    struct can_frame cf = SocketCANHelper::to_socketcan(frame);

    // Verify RTR flag is set
    REQUIRE((cf.can_id & CAN_RTR_FLAG) != 0);
    REQUIRE((cf.can_id & CAN_SFF_MASK) == 0x456);
    REQUIRE(cf.can_dlc == 0);
}

TEST_CASE("SocketCANHelper::to_socketcan - Zero DLC Frame", "[socketcan][conversion]") {
    // Create frame with DLC=0
    auto frame = VariableFrame(
        Format::DATA_VARIABLE,
        CANVersion::STD_VARIABLE,
        0x111,
        span<const std::uint8_t>()
    );

    // Convert to SocketCAN
    struct can_frame cf = SocketCANHelper::to_socketcan(frame);

    REQUIRE(cf.can_dlc == 0);
    REQUIRE((cf.can_id & CAN_SFF_MASK) == 0x111);
}

TEST_CASE("SocketCANHelper::to_socketcan - Max DLC (8 bytes)", "[socketcan][conversion]") {
    // Create frame with maximum data length
    std::vector<std::uint8_t> max_data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    auto frame = VariableFrame(
        Format::DATA_VARIABLE,
        CANVersion::EXT_VARIABLE,
        0x1FFFFFFF,  // Max extended ID
        span<const std::uint8_t>(max_data.data(), max_data.size())
    );

    // Convert to SocketCAN
    struct can_frame cf = SocketCANHelper::to_socketcan(frame);

    REQUIRE(cf.can_dlc == 8);
    for (int i = 0; i < 8; i++) {
        REQUIRE(cf.data[i] == max_data[i]);
    }
}

TEST_CASE("SocketCANHelper::from_socketcan - Standard ID Data Frame", "[socketcan][conversion]") {
    // Create SocketCAN frame
    struct can_frame cf;
    std::memset(&cf, 0, sizeof(cf));
    cf.can_id = 0x123;  // Standard ID, no flags
    cf.can_dlc = 3;
    cf.data[0] = 0x11;
    cf.data[1] = 0x22;
    cf.data[2] = 0x33;

    // Convert to VariableFrame
    VariableFrame frame = SocketCANHelper::from_socketcan(cf);

    // Verify frame properties
    REQUIRE(frame.get_can_id() == 0x123);
    REQUIRE(frame.is_extended() == false);
    REQUIRE(frame.get_format() == Format::DATA_VARIABLE);
    REQUIRE(frame.get_dlc() == 3);

    auto data = frame.get_data();
    REQUIRE(data.size() == 3);
    REQUIRE(data[0] == 0x11);
    REQUIRE(data[1] == 0x22);
    REQUIRE(data[2] == 0x33);
}

TEST_CASE("SocketCANHelper::from_socketcan - Extended ID Data Frame", "[socketcan][conversion]") {
    // Create SocketCAN frame with extended ID
    struct can_frame cf;
    std::memset(&cf, 0, sizeof(cf));
    cf.can_id = 0x12345678 | CAN_EFF_FLAG;  // Extended ID with flag
    cf.can_dlc = 4;
    cf.data[0] = 0xAA;
    cf.data[1] = 0xBB;
    cf.data[2] = 0xCC;
    cf.data[3] = 0xDD;

    // Convert to VariableFrame
    VariableFrame frame = SocketCANHelper::from_socketcan(cf);

    // Verify frame properties
    REQUIRE(frame.get_can_id() == 0x12345678);
    REQUIRE(frame.is_extended() == true);
    REQUIRE(frame.get_format() == Format::DATA_VARIABLE);
    REQUIRE(frame.get_dlc() == 4);

    auto data = frame.get_data();
    REQUIRE(data.size() == 4);
    REQUIRE(data[0] == 0xAA);
    REQUIRE(data[1] == 0xBB);
    REQUIRE(data[2] == 0xCC);
    REQUIRE(data[3] == 0xDD);
}

TEST_CASE("SocketCANHelper::from_socketcan - Remote Frame", "[socketcan][conversion]") {
    // Create SocketCAN remote frame
    struct can_frame cf;
    std::memset(&cf, 0, sizeof(cf));
    cf.can_id = 0x456 | CAN_RTR_FLAG;  // Standard ID with RTR flag
    cf.can_dlc = 0;  // Remote frames with DLC=0 for now

    // Convert to VariableFrame
    VariableFrame frame = SocketCANHelper::from_socketcan(cf);

    // Verify frame properties
    REQUIRE(frame.get_can_id() == 0x456);
    REQUIRE(frame.is_extended() == false);
    REQUIRE(frame.get_format() == Format::REMOTE_VARIABLE);
    REQUIRE(frame.get_dlc() == 0);
}

TEST_CASE("SocketCANHelper Round-Trip - Standard Data Frame",
    "[socketcan][conversion][roundtrip]") {
    // Create original VariableFrame
    auto original = VariableFrame(
        Format::DATA_VARIABLE,
        CANVersion::STD_VARIABLE,
        0x7FF,  // Max standard ID
        span<const std::uint8_t>({0x01, 0x02, 0x03, 0x04, 0x05})
    );

    // Convert to SocketCAN and back
    struct can_frame cf = SocketCANHelper::to_socketcan(original);
    VariableFrame restored = SocketCANHelper::from_socketcan(cf);

    // Verify round-trip preserves all fields
    REQUIRE(restored.get_can_id() == original.get_can_id());
    REQUIRE(restored.is_extended() == original.is_extended());
    REQUIRE(restored.get_format() == original.get_format());
    REQUIRE(restored.get_dlc() == original.get_dlc());

    auto orig_data = original.get_data();
    auto rest_data = restored.get_data();
    REQUIRE(orig_data.size() == rest_data.size());
    for (size_t i = 0; i < orig_data.size(); i++) {
        REQUIRE(orig_data[i] == rest_data[i]);
    }
}

TEST_CASE("SocketCANHelper Round-Trip - Extended Data Frame",
    "[socketcan][conversion][roundtrip]") {
    // Create original VariableFrame
    std::vector<std::uint8_t> test_data = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
    auto original = VariableFrame(
        Format::DATA_VARIABLE,
        CANVersion::EXT_VARIABLE,
        0x1FFFFFFF,  // Max extended ID
        span<const std::uint8_t>(test_data.data(), test_data.size())
    );

    // Convert to SocketCAN and back
    struct can_frame cf = SocketCANHelper::to_socketcan(original);
    VariableFrame restored = SocketCANHelper::from_socketcan(cf);

    // Verify round-trip preserves all fields
    REQUIRE(restored.get_can_id() == original.get_can_id());
    REQUIRE(restored.is_extended() == original.is_extended());
    REQUIRE(restored.get_format() == original.get_format());
    REQUIRE(restored.get_dlc() == original.get_dlc());

    auto orig_data = original.get_data();
    auto rest_data = restored.get_data();
    REQUIRE(orig_data.size() == rest_data.size());
    for (size_t i = 0; i < orig_data.size(); i++) {
        REQUIRE(orig_data[i] == rest_data[i]);
    }
}

TEST_CASE("SocketCANHelper::from_socketcan - Invalid DLC throws", "[socketcan][error]") {
    // Create SocketCAN frame with invalid DLC > 8
    struct can_frame cf;
    std::memset(&cf, 0, sizeof(cf));
    cf.can_id = 0x123;
    cf.can_dlc = 15;  // Invalid!

    // Should throw ProtocolException
    REQUIRE_THROWS_AS(SocketCANHelper::from_socketcan(cf), ProtocolException);
}

TEST_CASE("SocketCANHelper - Flag Verification", "[socketcan][flags]") {
    SECTION("Standard ID has no EFF flag") {
        auto frame = VariableFrame(Format::DATA_VARIABLE, CANVersion::STD_VARIABLE, 0x123, {});
        struct can_frame cf = SocketCANHelper::to_socketcan(frame);
        REQUIRE((cf.can_id & CAN_EFF_FLAG) == 0);
    }

    SECTION("Extended ID has EFF flag") {
        auto frame = VariableFrame(Format::DATA_VARIABLE, CANVersion::EXT_VARIABLE, 0x123456, {});
        struct can_frame cf = SocketCANHelper::to_socketcan(frame);
        REQUIRE((cf.can_id & CAN_EFF_FLAG) != 0);
    }

    SECTION("Data frame has no RTR flag") {
        std::vector<std::uint8_t> data_byte = {0x11};
        auto frame = VariableFrame(Format::DATA_VARIABLE, CANVersion::STD_VARIABLE, 0x123,
            span<const std::uint8_t>(data_byte.data(), data_byte.size()));
        struct can_frame cf = SocketCANHelper::to_socketcan(frame);
        REQUIRE((cf.can_id & CAN_RTR_FLAG) == 0);
    }

    SECTION("Remote frame has RTR flag") {
        auto frame = VariableFrame(Format::REMOTE_VARIABLE, CANVersion::STD_VARIABLE, 0x123, {});
        struct can_frame cf = SocketCANHelper::to_socketcan(frame);
        REQUIRE((cf.can_id & CAN_RTR_FLAG) != 0);
    }
}

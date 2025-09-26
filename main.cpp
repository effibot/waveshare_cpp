/**
 * @file main.cpp
 * @brief Example showcasing the Waveshare USB-CAN-A C++ Library
 *
 * This example demonstrates how to create, configure, serialize, and deserialize
 * different types of USB-CAN frames using the library. It shows the complete
 * roundtrip process: create frame -> set properties -> serialize -> deserialize.
 *
 * The example generates the following frame types with specific byte sequences:
 * - Variable Frame with standard ID format: AA C8 23 01 11 22 33 44 55 66 77 88 55
 * - Variable Frame with extended ID format: AA E8 67 45 23 01 11 22 33 44 55 66 77 88 55
 * - Fixed Frame with standard ID format: AA 55 01 01 01 01 23 01 00 08 11 22 33 44 55 66 77 88 00 93
 * - Fixed Frame with extended ID format: AA 55 01 02 01 78 56 34 12 08 01 02 03 04 05 06 07 08 00 44
 * - Config Frame: aa 55 12 01 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 14
 */

#include <iostream>
#include <iomanip>
#include <array>
#include <vector>

#include "include/common.hpp"
#include "include/variable_frame.hpp"
#include "include/fixed_frame.hpp"
#include "include/config_frame.hpp"
#include "include/frame_traits.hpp"

using namespace USBCANBridge;

/**
 * @brief Print a byte array in hexadecimal format
 * @param data The data to print
 * @param name A descriptive name for the data
 */
template<typename Container>
void print_hex(const Container& data, const std::string& name) {
    std::cout << name << ": ";
    for (const auto& byte : data) {
        std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                  << static_cast<unsigned int>(static_cast<uint8_t>(byte)) << " ";
    }
    std::cout << std::dec << std::endl;
}

/**
 * @brief Helper function to create byte array from hex values
 */
template<std::size_t N>
std::array<std::byte, N> make_bytes(const std::array<uint8_t, N>& values) {
    std::array<std::byte, N> result;
    for (std::size_t i = 0; i < N; ++i) {
        result[i] = static_cast<std::byte>(values[i]);
    }
    return result;
}

/**
 * @brief Helper function to create vector of bytes from hex values
 */
std::vector<std::byte> make_byte_vector(const std::vector<uint8_t>& values) {
    std::vector<std::byte> result;
    result.reserve(values.size());
    for (auto val : values) {
        result.push_back(static_cast<std::byte>(val));
    }
    return result;
}

/**
 * @brief Test Variable Frame with standard ID format
 * Expected: AA C8 23 01 11 22 33 44 55 66 77 88 55
 */
void test_variable_frame_standard() {
    std::cout << "\n=== Variable Frame with Standard ID ===\n";

    try {
        VariableFrame frame;
        using VarTraits = FrameTraits<VariableFrame>;

        // Set frame type first
        auto set_frame_type_result = frame.impl_set_frame_type(FrameType::STD_VAR);
        if (set_frame_type_result.fail()) {
            std::cout << "Failed to set frame type: " <<
                static_cast<int>(set_frame_type_result.status.value()) << std::endl;
            return;
        }

        // Set frame format
        auto set_frame_fmt_result = frame.impl_set_frame_fmt(FrameFmt::DATA_VAR);
        if (set_frame_fmt_result.fail()) {
            std::cout << "Failed to set frame format: " <<
                static_cast<int>(set_frame_fmt_result.status.value()) << std::endl;
            return;
        }

        // Set ID (0x0123 in little-endian: 23 01)
        VarTraits::IDType id_variant = std::array<std::byte, 2>{std::byte{0x23}, std::byte{0x01}};
        VarTraits::IDPair id_pair = {id_variant, 2};
        auto set_id_result = frame.impl_set_id(id_pair);
        if (set_id_result.fail()) {
            std::cout << "Failed to set ID: " << static_cast<int>(set_id_result.status.value()) <<
                std::endl;
            return;
        }

        // Set data payload (8 bytes: 11 22 33 44 55 66 77 88)
        std::vector<std::byte> data_bytes = make_byte_vector({0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                                                              0x77, 0x88});
        VarTraits::PayloadPair payload = {data_bytes, 8};
        auto set_data_result = frame.impl_set_data(payload);
        if (set_data_result.fail()) {
            std::cout << "Failed to set data: " <<
                static_cast<int>(set_data_result.status.value()) << std::endl;
            return;
        }

        // Serialize the frame
        auto serialize_result = frame.impl_serialize();
        if (serialize_result.fail()) {
            std::cout << "Failed to serialize: " <<
                static_cast<int>(serialize_result.status.value()) << std::endl;
            return;
        }

        auto serialized_data = serialize_result.value;
        print_hex(serialized_data, "Serialized Variable Frame (Standard)");

        // Create new frame and deserialize
        VariableFrame new_frame;
        auto deserialize_result = new_frame.impl_deserialize(serialized_data.data(),
            serialized_data.size());
        if (deserialize_result.fail()) {
            std::cout << "Failed to deserialize: " <<
                static_cast<int>(deserialize_result.status.value()) << std::endl;
            return;
        }

        // Verify by serializing again
        auto verify_result = new_frame.impl_serialize();
        if (verify_result.ok()) {
            print_hex(verify_result.value, "Deserialized Variable Frame (Standard)");
            std::cout << "Roundtrip test: " << (serialized_data ==
            verify_result.value ? "PASSED" : "FAILED") << std::endl;
        }

    } catch (const std::exception& e) {
        std::cout << "Exception in variable frame standard test: " << e.what() << std::endl;
    }
}

/**
 * @brief Test Variable Frame with extended ID format
 * Expected: AA E8 67 45 23 01 11 22 33 44 55 66 77 88 55
 */
void test_variable_frame_extended() {
    std::cout << "\n=== Variable Frame with Extended ID ===\n";

    try {
        VariableFrame frame;
        using VarTraits = FrameTraits<VariableFrame>;

        // Set frame type first
        auto set_frame_type_result = frame.impl_set_frame_type(FrameType::EXT_VAR);
        if (set_frame_type_result.fail()) {
            std::cout << "Failed to set frame type: " <<
                static_cast<int>(set_frame_type_result.status.value()) << std::endl;
            return;
        }

        // Set frame format
        auto set_frame_fmt_result = frame.impl_set_frame_fmt(FrameFmt::DATA_VAR);
        if (set_frame_fmt_result.fail()) {
            std::cout << "Failed to set frame format: " <<
                static_cast<int>(set_frame_fmt_result.status.value()) << std::endl;
            return;
        }

        // Set extended ID (0x01234567 in little-endian: 67 45 23 01)
        VarTraits::IDType id_variant = std::array<std::byte, 4>{std::byte{0x67}, std::byte{0x45},
                                                                std::byte{0x23}, std::byte{0x01}};
        VarTraits::IDPair id_pair = {id_variant, 4};
        auto set_id_result = frame.impl_set_id(id_pair);
        if (set_id_result.fail()) {
            std::cout << "Failed to set ID: " << static_cast<int>(set_id_result.status.value()) <<
                std::endl;
            return;
        }

        // Set data payload (8 bytes: 11 22 33 44 55 66 77 88)
        std::vector<std::byte> data_bytes = make_byte_vector({0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                                                              0x77, 0x88});
        VarTraits::PayloadPair payload = {data_bytes, 8};
        auto set_data_result = frame.impl_set_data(payload);
        if (set_data_result.fail()) {
            std::cout << "Failed to set data: " <<
                static_cast<int>(set_data_result.status.value()) << std::endl;
            return;
        }

        // Serialize the frame
        auto serialize_result = frame.impl_serialize();
        if (serialize_result.fail()) {
            std::cout << "Failed to serialize: " <<
                static_cast<int>(serialize_result.status.value()) << std::endl;
            return;
        }

        auto serialized_data = serialize_result.value;
        print_hex(serialized_data, "Serialized Variable Frame (Extended)");

        // Create new frame and deserialize
        VariableFrame new_frame;
        auto deserialize_result = new_frame.impl_deserialize(serialized_data.data(),
            serialized_data.size());
        if (deserialize_result.fail()) {
            std::cout << "Failed to deserialize: " <<
                static_cast<int>(deserialize_result.status.value()) << std::endl;
            return;
        }

        // Verify by serializing again
        auto verify_result = new_frame.impl_serialize();
        if (verify_result.ok()) {
            print_hex(verify_result.value, "Deserialized Variable Frame (Extended)");
            std::cout << "Roundtrip test: " << (serialized_data ==
            verify_result.value ? "PASSED" : "FAILED") << std::endl;
        }

    } catch (const std::exception& e) {
        std::cout << "Exception in variable frame extended test: " << e.what() << std::endl;
    }
}

/**
 * @brief Test Fixed Frame with standard ID format
 * Expected: AA 55 01 01 01 01 23 01 00 08 11 22 33 44 55 66 77 88 00 93
 */
void test_fixed_frame_standard() {
    std::cout << "\n=== Fixed Frame with Standard ID ===\n";

    try {
        FixedFrame frame;
        using FixedTraits = FrameTraits<FixedFrame>;

        // Set frame type to standard fixed
        auto set_frame_type_result = frame.impl_set_frame_type(FrameType::STD_FIXED);
        if (set_frame_type_result.fail()) {
            std::cout << "Failed to set frame type: " <<
                static_cast<int>(set_frame_type_result.status.value()) << std::endl;
            return;
        }

        // Set frame format to data fixed
        auto set_frame_fmt_result = frame.impl_set_frame_fmt(FrameFmt::DATA_FIXED);
        if (set_frame_fmt_result.fail()) {
            std::cout << "Failed to set frame format: " <<
                static_cast<int>(set_frame_fmt_result.status.value()) << std::endl;
            return;
        }

        // Set ID (0x000123 as 2-byte standard ID in little-endian: 23 01, padded: 01 23 01 00)
        std::array<std::byte, 4> id_bytes = make_bytes<4>({0x01, 0x23, 0x01, 0x00});
        FixedTraits::IDPair id_pair = {id_bytes, 2}; // Only 2 bytes for standard ID
        auto set_id_result = frame.impl_set_id(id_pair);
        if (set_id_result.fail()) {
            std::cout << "Failed to set ID: " << static_cast<int>(set_id_result.status.value()) <<
                std::endl;
            return;
        }

        // Set data payload (8 bytes: 11 22 33 44 55 66 77 88)
        std::array<std::byte,
            8> data_bytes = make_bytes<8>({0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88});
        FixedTraits::PayloadPair payload = {data_bytes, 8}; // DLC = 8
        auto set_data_result = frame.impl_set_data(payload);
        if (set_data_result.fail()) {
            std::cout << "Failed to set data: " <<
                static_cast<int>(set_data_result.status.value()) << std::endl;
            return;
        }

        // Serialize the frame
        auto serialize_result = frame.impl_serialize();
        if (serialize_result.fail()) {
            std::cout << "Failed to serialize: " <<
                static_cast<int>(serialize_result.status.value()) << std::endl;
            return;
        }

        auto serialized_data = serialize_result.value;
        print_hex(serialized_data, "Serialized Fixed Frame (Standard)");

        // Create new frame and deserialize
        FixedFrame new_frame;
        auto deserialize_result = new_frame.impl_deserialize(serialized_data);
        if (deserialize_result.fail()) {
            std::cout << "Failed to deserialize: " <<
                static_cast<int>(deserialize_result.status.value()) << std::endl;
            return;
        }

        // Verify by serializing again
        auto verify_result = new_frame.impl_serialize();
        if (verify_result.ok()) {
            print_hex(verify_result.value, "Deserialized Fixed Frame (Standard)");
            std::cout << "Roundtrip test: " << (serialized_data ==
            verify_result.value ? "PASSED" : "FAILED") << std::endl;
        }

    } catch (const std::exception& e) {
        std::cout << "Exception in fixed frame standard test: " << e.what() << std::endl;
    }
}

/**
 * @brief Test Fixed Frame with extended ID format
 * Expected: AA 55 01 02 01 78 56 34 12 08 01 02 03 04 05 06 07 08 00 44
 */
void test_fixed_frame_extended() {
    std::cout << "\n=== Fixed Frame with Extended ID ===\n";

    try {
        FixedFrame frame;
        using FixedTraits = FrameTraits<FixedFrame>;

        // Set frame type to extended fixed
        auto set_frame_type_result = frame.impl_set_frame_type(FrameType::EXT_FIXED);
        if (set_frame_type_result.fail()) {
            std::cout << "Failed to set frame type: " <<
                static_cast<int>(set_frame_type_result.status.value()) << std::endl;
            return;
        }

        // Set frame format to data fixed
        auto set_frame_fmt_result = frame.impl_set_frame_fmt(FrameFmt::DATA_FIXED);
        if (set_frame_fmt_result.fail()) {
            std::cout << "Failed to set frame format: " <<
                static_cast<int>(set_frame_fmt_result.status.value()) << std::endl;
            return;
        }

        // Set extended ID (0x12345678 in little-endian: 78 56 34 12)
        std::array<std::byte, 4> id_bytes = make_bytes<4>({0x78, 0x56, 0x34, 0x12});
        FixedTraits::IDPair id_pair = {id_bytes, 4}; // 4 bytes for extended ID
        auto set_id_result = frame.impl_set_id(id_pair);
        if (set_id_result.fail()) {
            std::cout << "Failed to set ID: " << static_cast<int>(set_id_result.status.value()) <<
                std::endl;
            return;
        }

        // Set data payload (8 bytes: 01 02 03 04 05 06 07 08)
        std::array<std::byte,
            8> data_bytes = make_bytes<8>({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08});
        FixedTraits::PayloadPair payload = {data_bytes, 8}; // DLC = 8
        auto set_data_result = frame.impl_set_data(payload);
        if (set_data_result.fail()) {
            std::cout << "Failed to set data: " <<
                static_cast<int>(set_data_result.status.value()) << std::endl;
            return;
        }

        // Serialize the frame
        auto serialize_result = frame.impl_serialize();
        if (serialize_result.fail()) {
            std::cout << "Failed to serialize: " <<
                static_cast<int>(serialize_result.status.value()) << std::endl;
            return;
        }

        auto serialized_data = serialize_result.value;
        print_hex(serialized_data, "Serialized Fixed Frame (Extended)");

        // Create new frame and deserialize
        FixedFrame new_frame;
        auto deserialize_result = new_frame.impl_deserialize(serialized_data);
        if (deserialize_result.fail()) {
            std::cout << "Failed to deserialize: " <<
                static_cast<int>(deserialize_result.status.value()) << std::endl;
            return;
        }

        // Verify by serializing again
        auto verify_result = new_frame.impl_serialize();
        if (verify_result.ok()) {
            print_hex(verify_result.value, "Deserialized Fixed Frame (Extended)");
            std::cout << "Roundtrip test: " << (serialized_data ==
            verify_result.value ? "PASSED" : "FAILED") << std::endl;
        }

    } catch (const std::exception& e) {
        std::cout << "Exception in fixed frame extended test: " << e.what() << std::endl;
    }
}

/**
 * @brief Test Configuration Frame
 * Expected: aa 55 12 01 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 14
 */
void test_config_frame() {
    std::cout << "\n=== Configuration Frame ===\n";

    try {
        ConfigFrame frame;

        // The config frame should already be initialized with default values
        // We just need to serialize it to see the output

        // Serialize the frame
        auto serialize_result = frame.impl_serialize();
        if (serialize_result.fail()) {
            std::cout << "Failed to serialize: " <<
                static_cast<int>(serialize_result.status.value()) << std::endl;
            return;
        }

        auto serialized_data = serialize_result.value;
        print_hex(serialized_data, "Serialized Config Frame");

        // Create new frame and deserialize
        ConfigFrame new_frame;
        auto deserialize_result = new_frame.impl_deserialize(serialized_data);
        if (deserialize_result.fail()) {
            std::cout << "Failed to deserialize: " <<
                static_cast<int>(deserialize_result.status.value()) << std::endl;
            return;
        }

        // Verify by serializing again
        auto verify_result = new_frame.impl_serialize();
        if (verify_result.ok()) {
            print_hex(verify_result.value, "Deserialized Config Frame");
            std::cout << "Roundtrip test: " << (serialized_data ==
            verify_result.value ? "PASSED" : "FAILED") << std::endl;
        }

    } catch (const std::exception& e) {
        std::cout << "Exception in config frame test: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "Waveshare USB-CAN-A C++ Library Showcase\n";
    std::cout << "=========================================\n";
    std::cout << "This example demonstrates creating, configuring, serializing,\n";
    std::cout << "and deserializing different types of USB-CAN frames.\n";

    // Test all frame types
    test_variable_frame_standard();
    test_variable_frame_extended();
    test_fixed_frame_standard();
    test_fixed_frame_extended();
    test_config_frame();

    std::cout << "\nShowcase completed.\n";
    return 0;
}
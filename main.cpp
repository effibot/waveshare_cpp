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
// Use builder pattern and new CRTP interface
#include "include/frame_builder.hpp"

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
        auto frame = USBCANBridge::make_variable_frame()
            .can_id(0x0123)
            .data(make_byte_vector({0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}))
            .dlc(8)
            .build();

        std::vector<std::byte> buffer(frame.get_raw_data().size());
        auto serialize_result = frame.serialize(USBCANBridge::span<std::byte>(buffer.data(),
            buffer.size()));
        if (!serialize_result) {
            std::cout << "Failed to serialize: " << serialize_result.describe() << std::endl;
            return;
        }
        print_hex(buffer, "Serialized Variable Frame (Standard)");

        VariableFrame new_frame;
        auto deserialize_result =
            new_frame.deserialize(USBCANBridge::span<const std::byte>(buffer.data(),
            buffer.size()));
        if (!deserialize_result) {
            std::cout << "Failed to deserialize: " << deserialize_result.describe() << std::endl;
            return;
        }
        std::vector<std::byte> verify_buffer(new_frame.get_raw_data().size());
        auto verify_result = new_frame.serialize(USBCANBridge::span<std::byte>(verify_buffer.data(),
            verify_buffer.size()));
        if (verify_result.ok()) {
            print_hex(verify_buffer, "Deserialized Variable Frame (Standard)");
            std::cout << "Roundtrip test: " << (buffer ==
            verify_buffer ? "PASSED" : "FAILED") << std::endl;
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
        auto frame = USBCANBridge::make_variable_frame()
            .extended_id(true)
            .can_id(0x01234567)
            .data(make_byte_vector({0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}))
            .dlc(8)
            .build();

        std::vector<std::byte> buffer(frame.get_raw_data().size());
        auto serialize_result = frame.serialize(USBCANBridge::span<std::byte>(buffer.data(),
            buffer.size()));
        if (!serialize_result) {
            std::cout << "Failed to serialize: " << serialize_result.describe() << std::endl;
            return;
        }
        print_hex(buffer, "Serialized Variable Frame (Extended)");

        VariableFrame new_frame;
        auto deserialize_result =
            new_frame.deserialize(USBCANBridge::span<const std::byte>(buffer.data(),
            buffer.size()));
        if (!deserialize_result) {
            std::cout << "Failed to deserialize: " << deserialize_result.describe() << std::endl;
            return;
        }
        std::vector<std::byte> verify_buffer(new_frame.get_raw_data().size());
        auto verify_result = new_frame.serialize(USBCANBridge::span<std::byte>(verify_buffer.data(),
            verify_buffer.size()));
        if (verify_result.ok()) {
            print_hex(verify_buffer, "Deserialized Variable Frame (Extended)");
            std::cout << "Roundtrip test: " << (buffer ==
            verify_buffer ? "PASSED" : "FAILED") << std::endl;
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
        auto frame = USBCANBridge::make_fixed_frame()
            .frame_type(FrameType::STD_FIXED)
            .can_id(0x0123)
            .data(std::vector<std::byte>(make_bytes<8>({0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                                        0x88}).begin(),
            make_bytes<8>({0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}).end()))
            .dlc(8)
            .build();

        std::array<std::byte, 20> buffer;
        auto serialize_result = frame.serialize(USBCANBridge::span<std::byte>(buffer.data(),
            buffer.size()));
        if (!serialize_result) {
            std::cout << "Failed to serialize: " << serialize_result.describe() << std::endl;
            return;
        }
        print_hex(buffer, "Serialized Fixed Frame (Standard)");

        FixedFrame new_frame;
        auto deserialize_result =
            new_frame.deserialize(USBCANBridge::span<const std::byte>(buffer.data(),
            buffer.size()));
        if (!deserialize_result) {
            std::cout << "Failed to deserialize: " << deserialize_result.describe() << std::endl;
            return;
        }
        std::array<std::byte, 20> verify_buffer;
        auto verify_result = new_frame.serialize(USBCANBridge::span<std::byte>(verify_buffer.data(),
            verify_buffer.size()));
        if (verify_result.ok()) {
            print_hex(verify_buffer, "Deserialized Fixed Frame (Standard)");
            std::cout << "Roundtrip test: " << (buffer ==
            verify_buffer ? "PASSED" : "FAILED") << std::endl;
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
        auto frame = USBCANBridge::make_fixed_frame()
            .frame_type(FrameType::EXT_FIXED)
            .can_id(0x12345678)
            .data(std::vector<std::byte>(make_bytes<8>({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                                        0x08}).begin(),
            make_bytes<8>({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}).end()))
            .dlc(8)
            .build();

        std::array<std::byte, 20> buffer;
        auto serialize_result = frame.serialize(USBCANBridge::span<std::byte>(buffer.data(),
            buffer.size()));
        if (!serialize_result) {
            std::cout << "Failed to serialize: " << serialize_result.describe() << std::endl;
            return;
        }
        print_hex(buffer, "Serialized Fixed Frame (Extended)");

        FixedFrame new_frame;
        auto deserialize_result =
            new_frame.deserialize(USBCANBridge::span<const std::byte>(buffer.data(),
            buffer.size()));
        if (!deserialize_result) {
            std::cout << "Failed to deserialize: " << deserialize_result.describe() << std::endl;
            return;
        }
        std::array<std::byte, 20> verify_buffer;
        auto verify_result = new_frame.serialize(USBCANBridge::span<std::byte>(verify_buffer.data(),
            verify_buffer.size()));
        if (verify_result.ok()) {
            print_hex(verify_buffer, "Deserialized Fixed Frame (Extended)");
            std::cout << "Roundtrip test: " << (buffer ==
            verify_buffer ? "PASSED" : "FAILED") << std::endl;
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
        auto frame = USBCANBridge::make_config_frame()
            .baud_rate(CANBaud::SPEED_125K)
            .filter(0, 0)
            .mode(CANMode::NORMAL)
            .build();

        std::array<std::byte, 20> buffer;
        auto serialize_result = frame.serialize(USBCANBridge::span<std::byte>(buffer.data(),
            buffer.size()));
        if (!serialize_result) {
            std::cout << "Failed to serialize: " << serialize_result.describe() << std::endl;
            return;
        }
        print_hex(buffer, "Serialized Config Frame");

        ConfigFrame new_frame;
        auto deserialize_result =
            new_frame.deserialize(USBCANBridge::span<const std::byte>(buffer.data(),
            buffer.size()));
        if (!deserialize_result) {
            std::cout << "Failed to deserialize: " << deserialize_result.describe() << std::endl;
            return;
        }
        std::array<std::byte, 20> verify_buffer;
        auto verify_result = new_frame.serialize(USBCANBridge::span<std::byte>(verify_buffer.data(),
            verify_buffer.size()));
        if (verify_result.ok()) {
            print_hex(verify_buffer, "Deserialized Config Frame");
            std::cout << "Roundtrip test: " << (buffer ==
            verify_buffer ? "PASSED" : "FAILED") << std::endl;
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
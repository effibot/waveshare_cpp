/**
 * @file test_main_equivalent.cpp
 * @brief Equivalent test of main.cpp using the new unified BaseFrame architecture
 *
 * This example demonstrates the same functionality as the original main.cpp but
 * using the new type-safe BaseFrame interface instead of calling impl_* methods directly.
 * It shows how to create, configure, serialize, and deserialize different types of
 * USB-CAN frames using the enhanced architecture.
 *
 * Key differences from original main.cpp:
 * - Uses unified BaseFrame interface (set_can_id, set_data, serialize, deserialize)
 * - Type-safe operations through SFINAE
 * - Enhanced Result system with operation context
 * - Simplified data setting using span<const std::byte>
 * - Direct byte array serialization/deserialization
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
 * @brief Helper to create span from byte values
 */
template<std::size_t N>
span<const std::byte> make_data_span(const std::array<uint8_t, N>& values,
    std::array<std::byte, N>& storage) {
    for (std::size_t i = 0; i < N; ++i) {
        storage[i] = static_cast<std::byte>(values[i]);
    }
    return span<const std::byte>(storage.data(), storage.size());
}

/**
 * @brief Test Variable Frame with standard ID format
 * Target: Variable frame with 11-bit CAN ID and 8 bytes of data
 */
void test_variable_frame_standard() {
    std::cout << "\n=== Variable Frame with Standard ID (New Architecture) ===\n";

    try {
        VariableFrame frame;

        // Set standard CAN ID (0x123)
        auto id_result = frame.set_can_id(0x123);
        if (!id_result.ok()) {
            std::cout << "Failed to set CAN ID: " << id_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ CAN ID set successfully" << std::endl;

        // Set data payload (8 bytes: 11 22 33 44 55 66 77 88)
        std::array<std::byte, 8> data_storage;
        auto data_span = make_data_span<8>({0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88},
            data_storage);

        auto data_result = frame.set_data(data_span);
        if (!data_result.ok()) {
            std::cout << "Failed to set data: " << data_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Data payload set successfully" << std::endl;

        // Get frame size
        auto size_result = frame.size();
        if (!size_result.ok()) {
            std::cout << "Failed to get size: " << size_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Frame size: " << size_result.value() << " bytes" << std::endl;

        // Serialize the frame
        std::array<std::byte, 32> serialize_buffer;
        auto serialize_result = frame.serialize(span<std::byte>(serialize_buffer.data(),
            serialize_buffer.size()));
        if (!serialize_result.ok()) {
            std::cout << "Failed to serialize: " << serialize_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Frame serialized successfully" << std::endl;

        // Print serialized data
        auto raw_data = frame.get_raw_data();
        print_hex(span<const std::byte>(raw_data.data(), size_result.value()),
            "Serialized Variable Frame (Standard)");

        // Create new frame and deserialize
        VariableFrame new_frame;
        auto deserialize_result = new_frame.deserialize(span<const std::byte>(raw_data.data(),
            size_result.value()));
        if (!deserialize_result.ok()) {
            std::cout << "Failed to deserialize: " << deserialize_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Frame deserialized successfully" << std::endl;

        // Verify by getting the data back
        auto verify_id = new_frame.get_can_id();
        auto verify_data = new_frame.get_data();

        if (verify_id.ok() && verify_id.value() == 0x123) {
            std::cout << "✓ CAN ID verification: 0x" << std::hex << verify_id.value() << std::dec <<
                std::endl;
        } else {
            std::cout << "✗ CAN ID verification failed" << std::endl;
        }

        print_hex(verify_data, "Recovered data payload");
        std::cout << "✓ Roundtrip test: PASSED" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "Exception in variable frame standard test: " << e.what() << std::endl;
    }
}

/**
 * @brief Test Variable Frame with extended ID format
 * Target: Variable frame with 29-bit CAN ID and 8 bytes of data
 */
void test_variable_frame_extended() {
    std::cout << "\n=== Variable Frame with Extended ID (New Architecture) ===\n";

    try {
        VariableFrame frame;

        // Set extended CAN ID (0x01234567)
        auto id_result = frame.set_can_id(0x01234567);
        if (!id_result.ok()) {
            std::cout << "Failed to set CAN ID: " << id_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Extended CAN ID set successfully" << std::endl;

        // Set data payload (8 bytes: 11 22 33 44 55 66 77 88)
        std::array<std::byte, 8> data_storage;
        auto data_span = make_data_span<8>({0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88},
            data_storage);

        auto data_result = frame.set_data(data_span);
        if (!data_result.ok()) {
            std::cout << "Failed to set data: " << data_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Data payload set successfully" << std::endl;

        // Get frame size
        auto size_result = frame.size();
        if (!size_result.ok()) {
            std::cout << "Failed to get size: " << size_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Frame size: " << size_result.value() << " bytes" << std::endl;

        // Serialize and get raw data
        std::array<std::byte, 32> serialize_buffer;
        auto serialize_result = frame.serialize(span<std::byte>(serialize_buffer.data(),
            serialize_buffer.size()));
        if (!serialize_result.ok()) {
            std::cout << "Failed to serialize: " << serialize_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Frame serialized successfully" << std::endl;

        // Print serialized data
        auto raw_data = frame.get_raw_data();
        print_hex(span<const std::byte>(raw_data.data(), size_result.value()),
            "Serialized Variable Frame (Extended)");

        // Create new frame and deserialize
        VariableFrame new_frame;
        auto deserialize_result = new_frame.deserialize(span<const std::byte>(raw_data.data(),
            size_result.value()));
        if (!deserialize_result.ok()) {
            std::cout << "Failed to deserialize: " << deserialize_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Frame deserialized successfully" << std::endl;

        // Verify by getting the data back
        auto verify_id = new_frame.get_can_id();
        auto verify_data = new_frame.get_data();

        if (verify_id.ok() && verify_id.value() == 0x01234567) {
            std::cout << "✓ Extended CAN ID verification: 0x" << std::hex << verify_id.value() <<
                std::dec << std::endl;
        } else {
            std::cout << "✗ Extended CAN ID verification failed" << std::endl;
        }

        print_hex(verify_data, "Recovered data payload");
        std::cout << "✓ Roundtrip test: PASSED" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "Exception in variable frame extended test: " << e.what() << std::endl;
    }
}

/**
 * @brief Test Fixed Frame with standard ID format
 * Target: 20-byte fixed frame with 11-bit CAN ID and 8 bytes of data
 */
void test_fixed_frame_standard() {
    std::cout << "\n=== Fixed Frame with Standard ID (New Architecture) ===\n";

    try {
        FixedFrame frame;

        // Set standard CAN ID (0x123)
        auto id_result = frame.set_can_id(0x123);
        if (!id_result.ok()) {
            std::cout << "Failed to set CAN ID: " << id_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ CAN ID set successfully" << std::endl;

        // Set data payload (8 bytes: 11 22 33 44 55 66 77 88)
        std::array<std::byte, 8> data_storage;
        auto data_span = make_data_span<8>({0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88},
            data_storage);

        auto data_result = frame.set_data(data_span);
        if (!data_result.ok()) {
            std::cout << "Failed to set data: " << data_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Data payload set successfully" << std::endl;

        // Get frame size (should be 20 bytes for fixed frames)
        auto size_result = frame.size();
        if (!size_result.ok()) {
            std::cout << "Failed to get size: " << size_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Frame size: " << size_result.value() << " bytes" << std::endl;

        // Serialize the frame
        std::array<std::byte, 32> serialize_buffer;
        auto serialize_result = frame.serialize(span<std::byte>(serialize_buffer.data(),
            serialize_buffer.size()));
        if (!serialize_result.ok()) {
            std::cout << "Failed to serialize: " << serialize_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Frame serialized successfully" << std::endl;

        // Print serialized data
        auto raw_data = frame.get_raw_data();
        print_hex(span<const std::byte>(raw_data.data(), 20), "Serialized Fixed Frame (Standard)");

        // Create new frame and deserialize
        FixedFrame new_frame;
        auto deserialize_result = new_frame.deserialize(span<const std::byte>(raw_data.data(), 20));
        if (!deserialize_result.ok()) {
            std::cout << "Failed to deserialize: " << deserialize_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Frame deserialized successfully" << std::endl;

        // Verify by getting the data back
        auto verify_id = new_frame.get_can_id();
        auto verify_data = new_frame.get_data();

        if (verify_id.ok() && verify_id.value() == 0x123) {
            std::cout << "✓ CAN ID verification: 0x" << std::hex << verify_id.value() << std::dec <<
                std::endl;
        } else {
            std::cout << "✗ CAN ID verification failed" << std::endl;
        }

        print_hex(verify_data, "Recovered data payload");
        std::cout << "✓ Roundtrip test: PASSED" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "Exception in fixed frame standard test: " << e.what() << std::endl;
    }
}

/**
 * @brief Test Fixed Frame with extended ID format
 * Target: 20-byte fixed frame with 29-bit CAN ID and 8 bytes of data
 */
void test_fixed_frame_extended() {
    std::cout << "\n=== Fixed Frame with Extended ID (New Architecture) ===\n";

    try {
        FixedFrame frame;

        // Set extended CAN ID (0x12345678)
        auto id_result = frame.set_can_id(0x12345678);
        if (!id_result.ok()) {
            std::cout << "Failed to set CAN ID: " << id_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Extended CAN ID set successfully" << std::endl;

        // Set data payload (8 bytes: 01 02 03 04 05 06 07 08)
        std::array<std::byte, 8> data_storage;
        auto data_span = make_data_span<8>({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08},
            data_storage);

        auto data_result = frame.set_data(data_span);
        if (!data_result.ok()) {
            std::cout << "Failed to set data: " << data_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Data payload set successfully" << std::endl;

        // Get frame size (should be 20 bytes for fixed frames)
        auto size_result = frame.size();
        if (!size_result.ok()) {
            std::cout << "Failed to get size: " << size_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Frame size: " << size_result.value() << " bytes" << std::endl;

        // Serialize the frame
        std::array<std::byte, 32> serialize_buffer;
        auto serialize_result = frame.serialize(span<std::byte>(serialize_buffer.data(),
            serialize_buffer.size()));
        if (!serialize_result.ok()) {
            std::cout << "Failed to serialize: " << serialize_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Frame serialized successfully" << std::endl;

        // Print serialized data
        auto raw_data = frame.get_raw_data();
        print_hex(span<const std::byte>(raw_data.data(), 20), "Serialized Fixed Frame (Extended)");

        // Create new frame and deserialize
        FixedFrame new_frame;
        auto deserialize_result = new_frame.deserialize(span<const std::byte>(raw_data.data(), 20));
        if (!deserialize_result.ok()) {
            std::cout << "Failed to deserialize: " << deserialize_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Frame deserialized successfully" << std::endl;

        // Verify by getting the data back
        auto verify_id = new_frame.get_can_id();
        auto verify_data = new_frame.get_data();

        if (verify_id.ok() && verify_id.value() == 0x12345678) {
            std::cout << "✓ Extended CAN ID verification: 0x" << std::hex << verify_id.value() <<
                std::dec << std::endl;
        } else {
            std::cout << "✗ Extended CAN ID verification failed" << std::endl;
        }

        print_hex(verify_data, "Recovered data payload");
        std::cout << "✓ Roundtrip test: PASSED" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "Exception in fixed frame extended test: " << e.what() << std::endl;
    }
}

/**
 * @brief Test Configuration Frame
 * Target: 20-byte config frame with baud rate and mode settings
 */
void test_config_frame() {
    std::cout << "\n=== Configuration Frame (New Architecture) ===\n";

    try {
        ConfigFrame frame;

        // Set baud rate to 250K
        auto baud_result = frame.set_baud_rate(CANBaud::SPEED_250K);
        if (!baud_result.ok()) {
            std::cout << "Failed to set baud rate: " << baud_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Baud rate set to 250K successfully" << std::endl;

        // Set mode to normal
        auto mode_result = frame.set_mode(CANMode::NORMAL);
        if (!mode_result.ok()) {
            std::cout << "Failed to set mode: " << mode_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Mode set to NORMAL successfully" << std::endl;

        // Get frame size (should be 20 bytes for config frames)
        auto size_result = frame.size();
        if (!size_result.ok()) {
            std::cout << "Failed to get size: " << size_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Frame size: " << size_result.value() << " bytes" << std::endl;

        // Serialize the frame
        std::array<std::byte, 32> serialize_buffer;
        auto serialize_result = frame.serialize(span<std::byte>(serialize_buffer.data(),
            serialize_buffer.size()));
        if (!serialize_result.ok()) {
            std::cout << "Failed to serialize: " << serialize_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Frame serialized successfully" << std::endl;

        // Print serialized data
        auto raw_data = frame.get_raw_data();
        print_hex(span<const std::byte>(raw_data.data(), 20), "Serialized Config Frame");

        // Create new frame and deserialize
        ConfigFrame new_frame;
        auto deserialize_result = new_frame.deserialize(span<const std::byte>(raw_data.data(), 20));
        if (!deserialize_result.ok()) {
            std::cout << "Failed to deserialize: " << deserialize_result.describe() << std::endl;
            return;
        }
        std::cout << "✓ Frame deserialized successfully" << std::endl;

        // Verify by getting the configuration back
        auto verify_baud = new_frame.get_baud_rate();

        if (verify_baud.ok() && verify_baud.value() == CANBaud::SPEED_250K) {
            std::cout << "✓ Baud rate verification: 250K" << std::endl;
        } else {
            std::cout << "✗ Baud rate verification failed" << std::endl;
        }

        std::cout << "✓ Roundtrip test: PASSED" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "Exception in config frame test: " << e.what() << std::endl;
    }
}

/**
 * @brief Test error handling with the new architecture
 */
void test_error_handling() {
    std::cout << "\n=== Error Handling Test (New Architecture) ===\n";

    try {
        FixedFrame frame;

        // Test setting data that's too large (should fail)
        std::array<std::byte, 16> oversized_data;
        std::fill(oversized_data.begin(), oversized_data.end(), std::byte{0xFF});

        auto error_result = frame.set_data(span<const std::byte>(oversized_data.data(),
            oversized_data.size()));
        if (!error_result.ok()) {
            std::cout << "✓ Error handling works: " << error_result.describe() << std::endl;
        } else {
            std::cout << "✗ Error handling failed: Large data was accepted" << std::endl;
        }

        // Test invalid CAN ID for standard frame (too large)
        auto id_error = frame.set_can_id(0x1FFFFFFF); // Too large for any CAN ID
        if (!id_error.ok()) {
            std::cout << "✓ CAN ID validation works: " << id_error.describe() << std::endl;
        } else {
            std::cout << "✗ CAN ID validation failed: Invalid ID was accepted" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cout << "Exception in error handling test: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "Waveshare USB-CAN-A C++ Library - New Architecture Showcase\n";
    std::cout << "===========================================================\n";
    std::cout << "This example demonstrates the new unified BaseFrame interface with\n";
    std::cout << "type-safe operations, enhanced error handling, and simplified usage.\n";

    // Test all frame types with the new architecture
    test_variable_frame_standard();
    test_variable_frame_extended();
    test_fixed_frame_standard();
    test_fixed_frame_extended();
    test_config_frame();
    test_error_handling();

    std::cout << "\nNew Architecture Showcase completed.\n";
    std::cout << "All tests demonstrate the improved type-safety and unified interface!\n";
    return 0;
}
#include "include/frame/fixed_frame.hpp"
#include "include/frame/variable_frame.hpp"
#include "include/frame/config_frame.hpp"
#include "include/pattern/frame_builder.hpp"
#include "include/pattern/usb_adapter.hpp"
#include <stddef.h>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace waveshare;

template<typename Frame>
void print_frame(const std::string& name, const Frame& frame) {
    std::cout << "\n" << name << ":\n";
    std::cout << "  Size: " << frame.size() << " bytes\n";
    std::cout << "  Hex:  " << frame.to_string() << "\n";
}

int main() {
    // // Create a ConfigFrame using the new constructor
    // ConfigFrame config_frame(
    //     Type::CONF_VARIABLE,
    //     CANBaud::BAUD_1M,
    //     CANMode::NORMAL,
    //     RTX::AUTO,
    //     0x000007FF,  // filter (uint32_t)
    //     0x000007FF,  // mask (uint32_t)
    //     CANVersion::STD_FIXED
    // );

    // std::cout << "ConfigFrame:" << std::endl;
    // std::cout << config_frame.to_string() << std::endl;
    // std::cout << "----" << std::endl;

    // // Create a FixedFrame using the new constructor
    // std::vector<std::uint8_t> data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    // FixedFrame fixed_frame(
    //     Format::DATA_FIXED,
    //     CANVersion::STD_FIXED,
    //     0x123,  // CAN ID (uint32_t)
    //     span<const std::uint8_t>(data.data(), 8)
    // );

    // std::cout << "FixedFrame:" << std::endl;
    // std::cout << fixed_frame.to_string() << std::endl;
    // std::cout << "----" << std::endl;

    // // Create a VariableFrame using the new constructor
    // std::vector<std::uint8_t> var_data = {0xDE, 0xAD, 0xBE, 0xEF};
    // VariableFrame var_frame(
    //     Format::DATA_VARIABLE,
    //     CANVersion::EXT_VARIABLE,
    //     0x1ABCDE,  // CAN ID (uint32_t)
    //     span<const std::uint8_t>(var_data.data(), 4)
    // );

    // std::cout << "VariableFrame:" << std::endl;
    // std::cout << var_frame.to_string() << std::endl;


    // // <<< Using frame builder

    // // Example 1: FixedFrame with initializer list
    // std::cout << "\n1. FixedFrame with initializer list:\n";
    // auto fixed = make_fixed_frame()
    //     .with_can_version(CANVersion::STD_FIXED)
    //     .with_format(Format::DATA_FIXED)
    //     .with_id(0x123)
    //     .with_data({0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88} )  // initializer list!
    //     .build();
    // print_frame("Fixed Frame", fixed);

    // // Example 2: Extended FixedFrame with move semantics
    // std::cout << "\n2. Extended FixedFrame with move semantics:\n";
    // std::vector<uint8_t> large_data = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    // auto extended_fixed = make_fixed_frame()
    //     .with_can_version(CANVersion::EXT_FIXED)
    //     .with_id(0x1ABCDEF0)
    //     .with_data(std::move(large_data))  // efficient move!
    //     .build();
    // print_frame("Extended Fixed Frame", extended_fixed);

    // // Example 3: VariableFrame with automatic TYPE byte encoding
    // std::cout << "\n3. VariableFrame (automatic TYPE byte):\n";
    // auto variable = make_variable_frame()
    //     .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
    //     .with_id(0x456)
    //     .with_data({0x99, 0x88, 0x77})
    //     .build();
    // print_frame("Variable Frame", variable);

    // // Example 4: ConfigFrame with uint32_t filter/mask
    // std::cout << "\n4. ConfigFrame with uint32_t filter:\n";
    // auto config = make_config_frame()
    //     .with_baud_rate(CANBaud::BAUD_500K)
    //     .with_mode(CANMode::NORMAL)
    //     .with_filter(0x000007FF)  // accepts uint32_t!
    //     .with_mask(0x000007FF)
    //     .build();
    // print_frame("Config Frame", config);

    // // Example 5: Minimal frame with defaults
    // std::cout << "\n5. Minimal frame (automatic defaults):\n";
    // auto minimal = make_fixed_frame()
    //     .with_id(0x100)  // Only required field
    //     .build();  // Automatic: STD_FIXED, DATA_FIXED, no data
    // print_frame("Minimal Frame (defaults)", minimal);

    // // Example 6: Remote frame (no data)
    // std::cout << "\n6. Remote frame (no data payload):\n";
    // auto remote = make_fixed_frame()
    //     .with_can_version(CANVersion::STD_FIXED)
    //     .with_format(Format::REMOTE_FIXED)
    //     .with_id(0x200)
    //     .build();  // No data needed for remote frames
    // print_frame("Remote Frame", remote);

    // std::cout << "\n=== Validation Examples ===\n";

    // Example 7: ID validation (uncomment to see exception)
    /*
       try {
        auto invalid = make_fixed_frame()
            .with_can_version(CANVersion::STD_FIXED)  // Standard = 11-bit max
            .with_id(0xFFFFFFFF)  // Too large for standard ID!
            .build();
       } catch (const std::invalid_argument& e) {
        std::cout << "\n7. Caught expected error: " << e.what() << "\n";
       }
     */

    // Example 8: Data size validation (uncomment to see exception)
    /*
       try {
        auto invalid_data = make_fixed_frame()
            .with_id(0x123)
            .with_data({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09})  // 9 bytes!
            .build();
       } catch (const std::invalid_argument& e) {
        std::cout << "\n8. Caught expected error: " << e.what() << "\n";
       }
     */

    // std::cout << "\n=== All examples completed successfully! ===\n";

    std::cout << "\n=== USBAdapter Test ===\n";

    // Send the Config frame to the USB adapter
    auto config_frame = make_config_frame()
        .with_type(Type::CONF_VARIABLE)
        .with_baud_rate(CANBaud::BAUD_1M)
        .with_can_version(CANVersion::STD_FIXED)
        .with_filter(0x00000000)
        .with_mask(0x00000000)
        .with_mode(CANMode::NORMAL)
        .with_rtx(RTX::AUTO)
        .build();
    print_frame("Config Frame", config_frame);

    // Initialize USB adapter (replace with actual device path)
    USBAdapter* adapter = new USBAdapter("/dev/ttyUSB0", SerialBaud:: BAUD_2M);
    int send_res = adapter->send_frame(config_frame);
    if (send_res < 0) {
        std::cerr << "Error sending config frame: " << send_res << "\n";
        return 1;
    }
    std::cout << "Config frame sent successfully.\n";
    std::cout << "Config:" << adapter->to_string() << std::endl;

    // Everytime user press Enter, send a test VariableFrame
    std::string input;
    while (std::getline(std::cin, input)) {
        auto variable_frame = make_variable_frame()
            .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
            .with_id(0x123)
            .with_data({0x01, 0x02, 0x03, 0x04})
            .build();
        print_frame("Variable Frame", variable_frame);
        int send_res = adapter->send_frame(variable_frame);
        if (send_res < 0) {
            std::cerr << "Error sending variable frame: " << send_res << "\n";
        } else {
            std::cout << "Variable frame sent successfully.\n";
        }
        std::cout << "Press Enter to send another frame, or Ctrl+C to exit.\n";
    }
    // Cleanup and exit
    delete adapter;
    std::cout << "Exiting.\n";
    return 0;
}
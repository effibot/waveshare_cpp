#include "include/frame/fixed_frame.hpp"
#include "include/frame/variable_frame.hpp"
#include "include/frame/config_frame.hpp"

#include <stddef.h>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace USBCANBridge;

int main() {
    // Create a ConfigFrame using the new constructor
    ConfigFrame config_frame(
        Type::CONF_VARIABLE,
        CANBaud::BAUD_1M,
        CANMode::NORMAL,
        RTX::AUTO,
        0x000007FF,  // filter (uint32_t)
        0x000007FF,  // mask (uint32_t)
        CANVersion::STD_FIXED
    );

    std::cout << "ConfigFrame:" << std::endl;
    std::cout << config_frame.to_string() << std::endl;
    std::cout << "----" << std::endl;

    // Create a FixedFrame using the new constructor
    std::vector<std::uint8_t> data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    FixedFrame fixed_frame(
        Format::DATA_FIXED,
        CANVersion::STD_FIXED,
        0x123,  // CAN ID (uint32_t)
        span<const std::uint8_t>(data.data(), 8)
    );

    std::cout << "FixedFrame:" << std::endl;
    std::cout << fixed_frame.to_string() << std::endl;
    std::cout << "----" << std::endl;

    // Create a VariableFrame using the new constructor
    std::vector<std::uint8_t> var_data = {0xDE, 0xAD, 0xBE, 0xEF};
    VariableFrame var_frame(
        Format::DATA_VARIABLE,
        CANVersion::EXT_VARIABLE,
        0x1ABCDE,  // CAN ID (uint32_t)
        span<const std::uint8_t>(var_data.data(), 4)
    );

    std::cout << "VariableFrame:" << std::endl;
    std::cout << var_frame.to_string() << std::endl;

    return 0;
}
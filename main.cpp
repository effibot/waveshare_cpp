#include "include/pattern/frame_builder.hpp"

#include <stddef.h>
#include <cstdint>
#include <iostream>
#include <string>

using namespace USBCANBridge;
int main() {
    std::array<std::uint8_t, 4> filter = {0x00, 0x00, 0x07, 0xFF};
    std::array<std::uint8_t, 4> mask = {0x00, 0x00, 0x07, 0xFF};
    FrameBuilder<ConfigFrame> builder;
    ConfigFrame frame = builder
        .type(Type::CONF_VARIABLE)
        .baud_rate(CANBaud::BAUD_1M)
        .mode(CANMode::NORMAL)
        .filter(filter)
        .mask(mask)
        .rtx(RTX::AUTO)
        .can_version(CANVersion::STD_FIXED)
        .finalize()
        .build();

    std::string frame_dump = frame.to_string();
    std::cout << frame_dump << std::endl;
    std::cout << "----" << std::endl;

    FrameBuilder<FixedFrame> fixed_builder;
    FixedFrame fixed_frame = fixed_builder
        .type(Type::DATA_FIXED)
        .can_version(CANVersion::STD_FIXED)
        .format(Format::DATA_FIXED)
        .id(0x123)
        .data({0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88})
        .finalize()
        .build();

    std::cout << fixed_frame.to_string() << std::endl;
    std::cout << "----" << std::endl;

    FrameBuilder<VariableFrame> var_builder;
    VariableFrame var_frame = var_builder
        .type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
        .id(0x1ABCDE)
        .data({0xDE, 0xAD, 0xBE, 0xEF})
        .finalize()
        .build();
    return 0;
}
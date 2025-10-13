/**
 * @file wave_writer.cpp
 * @author effibot (andrea.efficace1@gmail.com)
 * @brief Simple example to write frames to the Waveshare USB-CAN adapter
 * @version 0.1
 * @date 2025-10-13
 *
 * @copyright Copyright (c) 2025
 *
 */


#include "../include/waveshare.hpp"
#include <chrono>
#include <thread>
using namespace USBCANBridge;

void display_help(std::string argv) {
    std::cout << "Usage: " << argv << " [-d device] [-b baudrate]\n";
    std::cout << "\t-d Device   Serial device (default: /dev/ttyUSB0)\n";
    std::cout <<
        "\t-b Baudrate Baudrate (9600, 19200, 38400, 57600, 115200, 153600, 2000000) (default: 2000000)\n";
    std::cout << "\t-f Fixed/variable frame to write (default: variable)\n";
    std::cout << "\t-h Display this help message\n";
}

int main(int argc, char* argv[]) {
    // Default parameters
    std::string device = "/dev/ttyUSB1"; // Default device
    SerialBaud baudrate = SerialBaud::BAUD_2M;     // Default
    CANBaud can_baudrate = CANBaud::BAUD_1M; // Default CAN baudrate
    // Parse command-line arguments
    int arg_index;
    std::string tty_device;
    SerialBaud tty_baudrate;
    CANBaud tty_can_baudrate;
    bool read_fixed = false;
    while ((arg_index = getopt(argc, argv, "hd:s:b:f:")) != -1) {
        switch (arg_index) {
        case 'h':
            display_help(argv[0]);
            return 0;
        case 'd':
            device = optarg;
            break;
        case 's':
            try {
                int baud = std::stoi(optarg);
                switch (baud) {
                case 9600: tty_baudrate = SerialBaud::BAUD_9600; break;
                case 19200: tty_baudrate = SerialBaud::BAUD_19200; break;
                case 38400: tty_baudrate = SerialBaud::BAUD_38400; break;
                case 57600: tty_baudrate = SerialBaud::BAUD_57600; break;
                case 115200: tty_baudrate = SerialBaud::BAUD_115200; break;
                case 153600: tty_baudrate = SerialBaud::BAUD_153600; break;
                case 2000000: tty_baudrate = SerialBaud::BAUD_2M; break;
                default:
                    std::cerr <<
                        "Invalid baudrate. Supported values: 9600, 19200, 38400, 57600, 115200, 153600, 2000000\n";
                    return 1;
                }
                baudrate = tty_baudrate;
            } catch (const std::invalid_argument&) {
                std::cerr << "Invalid baudrate format.\n";
                return 1;
            }
            break;
        case 'b':
            try {
                int can_baud = std::stoi(optarg);
                switch (can_baud) {
                case 10000: tty_can_baudrate = CANBaud::BAUD_10K; break;
                case 20000: tty_can_baudrate = CANBaud::BAUD_20K; break;
                case 50000: tty_can_baudrate = CANBaud::BAUD_50K; break;
                case 100000: tty_can_baudrate = CANBaud::BAUD_100K; break;
                case 125000: tty_can_baudrate = CANBaud::BAUD_125K; break;
                case 200000: tty_can_baudrate = CANBaud::BAUD_200K; break;
                case 250000: tty_can_baudrate = CANBaud ::BAUD_250K; break;
                case 400000: tty_can_baudrate = CANBaud::BAUD_400K; break;
                case 500000: tty_can_baudrate = CANBaud::BAUD_500K; break;
                case 800000: tty_can_baudrate = CANBaud::BAUD_800K; break;
                case 1000000: tty_can_baudrate = CANBaud::BAUD_1M; break;
                default:
                    std::cerr <<
                        "Invalid CAN baudrate. Supported values: 10000, 20000, 50000, 100000, 125000, 200000, 250000, 400000, 500000, 800000, 1000000\n";
                    return 1;
                }
                can_baudrate = tty_can_baudrate;
            }       catch (const std::invalid_argument&) {
                std::cerr << "Invalid CAN baudrate format.\n";
                return 1;
            }
            break;
        case 'f':
            if (std::string(optarg) == "fixed") {
                read_fixed = true;
            } else if (std::string(optarg) == "variable") {
                read_fixed = false;
            } else {
                std::cerr << "Invalid frame type. Use 'fixed' or 'variable'.\n";
                return 1;
            }
            break;
        case '?':
            if (optopt == 'd' || optopt == 'b' || optopt == 's' || optopt == 'f') {
                std::cerr << "Option -" << static_cast<char>(optopt) << " requires an argument.\n";
            } else {
                std::cerr << "Unknown option `-" << static_cast<char>(optopt) << "`.\n";
            }
            return 1;
        default:
            display_help(argv[0]);
            return 1;
        }
    }

    std::cout << "Using device: " << device << ", baudrate: " << static_cast<int>(baudrate) <<
        std::endl;
    // Initialize the adapter
    USBAdapter* adapter = new USBAdapter(device, baudrate);
    if (!adapter->is_open()) {
        std::cerr << "Failed to open device: " << device << "\n";
        delete adapter;
        return 1;
    }
    std::cout << "Device opened successfully.\n";
    // Configure the CAN bus
    ConfigFrame config_frame;
    if (read_fixed) {
        config_frame = make_config_frame()
            .with_type(Type::CONF_FIXED)
            .with_baud_rate(can_baudrate)
            .with_mode(CANMode::NORMAL)
            .with_can_version(CANVersion::STD_FIXED)
            .with_filter(0x00000000)
            .with_mask(0x00000000)
            .with_rtx(RTX::OFF)
            .build();
    } else {
        config_frame = make_config_frame()
            .with_type(Type::CONF_VARIABLE)
            .with_baud_rate(can_baudrate)
            .with_can_version(CANVersion::STD_FIXED)
            .with_filter(0x00000000)
            .with_mask(0x00000000)
            .with_mode(CANMode::NORMAL)
            .with_rtx(RTX::OFF)
            .build();
    }
    auto send_res = adapter->send_frame(config_frame);
    if (send_res.fail()) {
        std::cerr << "Error sending config frame: " << send_res.describe() << "\n";
        delete adapter;
        return 1;
    }
    std::cout << "Config frame sent successfully.\n";
    std::cout << "Config >>\t" << config_frame.to_string() << std::endl;

    //  Find the frame type to use


}
#include "../include/waveshare.hpp"
#include "script_utils.hpp"
#include <iostream>
#include <iomanip>
#include <csignal>
#include <chrono>
#include <thread>
#include <sstream>
#include <linux/can.h>

using namespace waveshare;

// Global bridge pointer for signal handler
SocketCANBridge* g_bridge = nullptr;

// Helper function to format CAN frame as hex string
std::string format_can_data(const uint8_t* data, uint8_t len) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0');
    for (uint8_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]);
        if (i < len - 1) oss << " ";
    }
    return oss.str();
}

// Helper function to get current timestamp string
std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt = *std::localtime(&timer);

    std::ostringstream oss;
    oss << std::put_time(&bt, "%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n\n[SIGNAL] Received SIGINT (Ctrl+C) - shutting down gracefully...\n";
        if (g_bridge && g_bridge->is_running()) {
            g_bridge->stop();
            std::cout << "[SIGNAL] Bridge stopped.\n";
        }
        std::exit(0);
    }
}

void display_help(const std::string& program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -i <interface> SocketCAN interface (default: vcan0)\n";
    std::cout << "  -d <device>    USB device path (default: /dev/ttyUSB0)\n";
    std::cout << "  -s <baudrate>  Serial baudrate (default: 2000000)\n";
    std::cout << "                 Supported: 9600, 19200, 38400, 57600, 115200, 153600, 2000000\n";
    std::cout << "  -c <baudrate>  CAN bus baudrate (default: 1000000)\n";
    std::cout << "                 Supported: 10000, 20000, 50000, 100000, 125000, 200000,\n";
    std::cout << "                            250000, 400000, 500000, 800000, 1000000\n";
    std::cout << "  -h             Display this help message\n";
    std::cout << "\n";
    std::cout << "Example:\n";
    std::cout << "  " << program_name << " -i vcan0 -d /dev/ttyUSB0 -s 2000000 -c 1000000\n";
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    std::cout << "=== SocketCAN Bridge Manual Test ===\n\n";

    try {
        // Default configuration
        std::string socketcan_interface = "vcan0";
        ScriptConfig script_config;
        script_config.device = "/dev/ttyUSB0";
        script_config.serial_baudrate = SerialBaud::BAUD_2M;
        script_config.can_baudrate = CANBaud::BAUD_1M;
        script_config.use_fixed_frames = false;

        // Parse command-line arguments
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "-h") {
                display_help(argv[0]);
                return 0;
            } else if (arg == "-i" && i + 1 < argc) {
                socketcan_interface = argv[++i];
            } else if (arg == "-d" && i + 1 < argc) {
                script_config.device = argv[++i];
            } else if (arg == "-s" && i + 1 < argc) {
                int serial_baudrate = std::stoi(argv[++i]);
                bool baud_not_found = false;
                script_config.serial_baudrate = serialbaud_from_int(serial_baudrate,
                    baud_not_found);
                if (baud_not_found) {
                    throw std::invalid_argument("Unsupported serial baudrate: " +
                        std::to_string(serial_baudrate));
                }
            } else if (arg == "-c" && i + 1 < argc) {
                int can_baudrate = std::stoi(argv[++i]);
                bool baud_not_found = false;
                script_config.can_baudrate = canbaud_from_int(can_baudrate, baud_not_found);
                if (baud_not_found) {
                    throw std::invalid_argument("Unsupported CAN baudrate: " +
                        std::to_string(can_baudrate));
                }
            } else {
                std::cerr << "Unknown argument: " << arg << "\n";
                display_help(argv[0]);
                return 1;
            }
        }

        std::cout << "Configuration:\n";
        std::cout << "  SocketCAN Interface: " << socketcan_interface << "\n";
        std::cout << "  USB Device:          " << script_config.device << "\n";
        std::cout << "  Serial Baud:         " << static_cast<int>(script_config.serial_baudrate) <<
            " bps\n";
        std::cout << "  CAN Baud:            " << static_cast<int>(script_config.can_baudrate) <<
            " bps\n";
        std::cout << "  CAN Mode:            NORMAL\n\n";

        // Create bridge configuration
        BridgeConfig config = BridgeConfig::create_default();
        config.socketcan_interface = socketcan_interface;
        config.usb_device_path = script_config.device;
        config.serial_baud_rate = script_config.serial_baudrate;
        config.can_baud_rate = script_config.can_baudrate;
        config.can_mode = CANMode::NORMAL;
        config.auto_retransmit = true;
        config.usb_read_timeout_ms = 100;
        config.socketcan_read_timeout_ms = 100;

        // Validate configuration
        config.validate();
        std::cout << "[CONFIG] Configuration validated successfully.\n\n";

        // Create bridge using factory method
        std::cout << "[BRIDGE] Creating SocketCAN bridge...\n";
        auto bridge_ptr = SocketCANBridge::create(config);
        SocketCANBridge& bridge = *bridge_ptr;
        g_bridge = bridge_ptr.get();

        if (bridge.is_socketcan_open()) {
            std::cout << "[BRIDGE] SocketCAN socket opened successfully\n";
        }
        if (bridge.get_adapter()) {
            std::cout << "[BRIDGE] USB adapter initialized successfully\n";
        }

        // Install signal handler
        std::signal(SIGINT, signal_handler);
        std::cout << "[BRIDGE] Signal handler installed (Ctrl+C to stop)\n\n";

        // Set up frame dump callbacks
        std::cout << "[BRIDGE] Installing frame dump callbacks...\n";

        bridge.set_usb_to_socketcan_callback(
            [](const VariableFrame& usb_frame, const ::can_frame& socketcan_frame) {
                std::cout << "[" << get_timestamp() << "] USB→CAN: "
                          << "ID=0x" << std::hex << std::uppercase << std::setfill('0')
                          << std::setw(usb_frame.is_extended() ? 8 : 3) << usb_frame.get_can_id()
                          << std::dec << " DLC=" << static_cast<int>(usb_frame.get_dlc())
                          << " DATA=[" << format_can_data(usb_frame.get_data().data(),
                usb_frame.get_dlc()) << "]"
                          << " → CAN ID=0x" << std::hex << std::uppercase
                          << std::setw((socketcan_frame.can_id & CAN_EFF_FLAG) ? 8 : 3)
                          << (socketcan_frame.can_id & CAN_EFF_MASK)
                          << std::dec << " DLC=" << static_cast<int>(socketcan_frame.can_dlc)
                          << std::endl;
            }
        );

        bridge.set_socketcan_to_usb_callback(
            [](const ::can_frame& socketcan_frame, const VariableFrame& usb_frame) {
                std::cout << "[" << get_timestamp() << "] CAN→USB: "
                          << "ID=0x" << std::hex << std::uppercase << std::setfill('0')
                          << std::setw((socketcan_frame.can_id & CAN_EFF_FLAG) ? 8 : 3)
                          << (socketcan_frame.can_id & CAN_EFF_MASK)
                          << std::dec << " DLC=" << static_cast<int>(socketcan_frame.can_dlc)
                          << " DATA=[" << format_can_data(socketcan_frame.data,
                socketcan_frame.can_dlc) << "]"
                          << " → USB ID=0x" << std::hex << std::uppercase
                          << std::setw(usb_frame.is_extended() ? 8 : 3) << usb_frame.get_can_id()
                          << std::dec << " DLC=" << static_cast<int>(usb_frame.get_dlc())
                          << std::endl;
            }
        );

        std::cout << "[BRIDGE] Frame dump callbacks installed.\n\n";

        // Start bridge
        std::cout << "[BRIDGE] Starting forwarding threads...\n";
        bridge.start();
        std::cout << "[BRIDGE] Bridge is running! Forwarding frames bidirectionally.\n";
        std::cout << "[BRIDGE] Status: " << (bridge.is_running() ? "RUNNING" : "STOPPED") << "\n\n";

        std::cout << "=== Bridge Active ===\n";
        std::cout << "The bridge is now forwarding frames between USB and SocketCAN.\n";
        std::cout << "Frame dumps will appear in real-time below.\n\n";
        std::cout << "Test commands (run in another terminal):\n";
        std::cout << "  # Send frame to SocketCAN (will be forwarded to USB):\n";
        std::cout << "  cansend " << socketcan_interface << " 123#DEADBEEF\n\n";
        std::cout << "  # Monitor SocketCAN frames (will show frames from USB):\n";
        std::cout << "  candump " << socketcan_interface << "\n\n";
        std::cout << "Press Ctrl+C to stop and show statistics.\n";
        std::cout << "========================================\n\n";

        // Main loop - print statistics every 10 seconds
        auto last_stats_time = std::chrono::steady_clock::now();
        while (bridge.is_running()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now -
                last_stats_time).count();

            if (elapsed >= 10) {
                auto stats = bridge.get_statistics();
                std::cout << "\n--- Statistics (10s update) ---\n";
                std::cout << stats.to_string() << "\n";
                std::cout << "------------------------------\n\n";
                last_stats_time = now;
            }
        }

        // Should not reach here unless stopped externally
        std::cout << "\n[BRIDGE] Bridge stopped.\n";

    } catch (const DeviceException& e) {
        std::cerr << "\n[ERROR] Device error: " << e.what() << "\n";
        std::cerr << "  Status code: " << static_cast<int>(e.status()) << "\n";
        std::cerr << "\nTroubleshooting:\n";
        std::cerr << "  - Check USB device path (default: /dev/ttyUSB0)\n";
        std::cerr << "  - Check SocketCAN interface (default: vcan0)\n";
        std::cerr <<
            "  - For vcan0: sudo modprobe vcan && sudo ip link add dev vcan0 type vcan && sudo ip link set up vcan0\n";
        return 1;
    }
    catch (const std::invalid_argument& e) {
        std::cerr << "\n[ERROR] Configuration error: " << e.what() << "\n";
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "\n[ERROR] Unexpected error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\n[MAIN] Exiting.\n";
    return 0;
}

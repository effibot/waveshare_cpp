#include "include/pattern/socketcan_bridge.hpp"
#include "include/pattern/bridge_config.hpp"
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

int main(int argc, char* argv[]) {
    std::cout << "=== SocketCAN Bridge Manual Test ===\n\n";

    try {
        // Parse command-line arguments for interface and device
        std::string socketcan_interface = "vcan0";
        std::string usb_device = "/dev/ttyUSB0";

        if (argc > 1) {
            socketcan_interface = argv[1];
        }
        if (argc > 2) {
            usb_device = argv[2];
        }

        std::cout << "Configuration:\n";
        std::cout << "  SocketCAN Interface: " << socketcan_interface << "\n";
        std::cout << "  USB Device:          " << usb_device << "\n";
        std::cout << "  Serial Baud:         2000000 (2Mbps)\n";
        std::cout << "  CAN Baud:            1000000 (1Mbps)\n";
        std::cout << "  CAN Mode:            NORMAL\n\n";

        // Create configuration
        BridgeConfig config = BridgeConfig::create_default();
        config.socketcan_interface = socketcan_interface;
        config.usb_device_path = usb_device;
        config.serial_baud_rate = SerialBaud::BAUD_2M;
        config.can_baud_rate = CANBaud::BAUD_1M;
        config.can_mode = CANMode::NORMAL;
        config.auto_retransmit = true;
        config.usb_read_timeout_ms = 100;
        config.socketcan_read_timeout_ms = 100;

        // Validate configuration
        config.validate();
        std::cout << "[CONFIG] Configuration validated successfully.\n\n";

        // Create bridge
        std::cout << "[BRIDGE] Creating SocketCAN bridge...\n";
        SocketCANBridge bridge(config);
        g_bridge = &bridge;

        if (bridge.is_socketcan_open()) {
            std::cout << "[BRIDGE] SocketCAN socket opened successfully (fd="
                      << bridge.get_socketcan_fd() << ")\n";
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

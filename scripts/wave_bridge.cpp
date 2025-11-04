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

// === Callback Functions ===

/**
 * @brief Callback for USB to SocketCAN frame forwarding
 * @param usb_frame The Waveshare frame received from USB
 * @param socketcan_frame The SocketCAN frame to be sent
 */
void usb_to_socketcan_callback(const VariableFrame& usb_frame, const ::can_frame& socketcan_frame) {
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

/**
 * @brief Callback for SocketCAN to USB frame forwarding
 * @param socketcan_frame The SocketCAN frame received
 * @param usb_frame The Waveshare frame to be sent to USB
 */
void socketcan_to_usb_callback(const ::can_frame& socketcan_frame, const VariableFrame& usb_frame) {
    std::cout << "[" << get_timestamp() << "] CAN→USB: "
              << "ID=0x" << std::hex << std::uppercase << std::setfill('0')
              << std::setw((socketcan_frame.can_id & CAN_EFF_FLAG) ? 8 : 3)
              << (socketcan_frame.can_id & CAN_EFF_MASK)
              << std::dec << " DLC=" << static_cast<int>(socketcan_frame.can_dlc)
              << " DATA=[" << format_can_data(socketcan_frame.data, socketcan_frame.can_dlc) << "]"
              << " → USB ID=0x" << std::hex << std::uppercase
              << std::setw(usb_frame.is_extended() ? 8 : 3) << usb_frame.get_can_id()
              << std::dec << " DLC=" << static_cast<int>(usb_frame.get_dlc())
              << std::endl;
}

// === Main Program ===

// === Main Program ===

int main(int argc, char* argv[]) {
    std::cout << "=== SocketCAN Bridge Manual Test ===\n\n";

    try {
        // Parse command-line arguments using standardized parser
        ScriptConfig config = parse_arguments(argc, argv, ScriptType::BRIDGE);

        std::cout << "Configuration:\n";
        std::cout << "  SocketCAN Interface: " << config.socketcan_interface << "\n";
        std::cout << "  USB Device:          " << config.device << "\n";
        std::cout << "  Serial Baud:         " << static_cast<int>(config.serial_baudrate) <<
            " bps\n";
        std::cout << "  CAN Baud:            " << static_cast<int>(config.can_baudrate) << " bps\n";

        // Display CAN mode
        std::cout << "  CAN Mode:            ";
        switch (config.can_mode) {
        case CANMode::NORMAL:          std::cout << "NORMAL\n"; break;
        case CANMode::LOOPBACK:        std::cout << "LOOPBACK\n"; break;
        case CANMode::SILENT:          std::cout << "SILENT\n"; break;
        case CANMode::LOOPBACK_SILENT: std::cout << "LOOPBACK_SILENT\n"; break;
        default:                       std::cout << "UNKNOWN\n"; break;
        }

        std::cout << "  Auto-Retransmit:     " << (config.auto_retransmit ? "ON" : "OFF") << "\n";
        std::cout << "  Filter ID:           0x" << std::hex << std::uppercase << std::setfill('0')
                  << std::setw(8) << config.filter_id << std::dec << "\n";
        std::cout << "  Filter Mask:         0x" << std::hex << std::uppercase << std::setfill('0')
                  << std::setw(8) << config.filter_mask << std::dec << "\n";
        std::cout << "  USB Read Timeout:    " << config.usb_read_timeout_ms << " ms\n";
        std::cout << "  SocketCAN Timeout:   " << config.socketcan_read_timeout_ms << " ms\n\n";

        // Create bridge configuration from parsed arguments
        BridgeConfig bridge_config = BridgeConfig::create_default();
        bridge_config.socketcan_interface = config.socketcan_interface;
        bridge_config.usb_device_path = config.device;
        bridge_config.serial_baud_rate = config.serial_baudrate;
        bridge_config.can_baud_rate = config.can_baudrate;
        bridge_config.can_mode = config.can_mode;
        bridge_config.auto_retransmit = config.auto_retransmit;
        bridge_config.filter_id = config.filter_id;
        bridge_config.filter_mask = config.filter_mask;
        bridge_config.usb_read_timeout_ms = config.usb_read_timeout_ms;
        bridge_config.socketcan_read_timeout_ms = config.socketcan_read_timeout_ms;

        // Validate configuration
        bridge_config.validate();
        std::cout << "[CONFIG] Configuration validated successfully.\n\n";

        // Create bridge using factory method
        std::cout << "[BRIDGE] Creating SocketCAN bridge...\n";
        auto bridge_ptr = SocketCANBridge::create(bridge_config);
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

        // Set up frame dump callbacks using external functions
        std::cout << "[BRIDGE] Installing frame dump callbacks...\n";
        bridge.set_usb_to_socketcan_callback(usb_to_socketcan_callback);
        bridge.set_socketcan_to_usb_callback(socketcan_to_usb_callback);
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
        std::cout << "  cansend " << config.socketcan_interface << " 123#DEADBEEF\n\n";
        std::cout << "  # Monitor SocketCAN frames (will show frames from USB):\n";
        std::cout << "  candump " << config.socketcan_interface << "\n\n";
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

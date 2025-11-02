/**
 * @file can_bus_scanner.cpp
 * @brief CAN bus diagnostic and device scanner
 * @version 1.0
 * @date 2025-11-02
 *
 * Scans for devices on the CAN bus and tests different configurations
 */

#include "../include/waveshare.hpp"
#include "script_utils.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <vector>
#include <set>

using namespace waveshare;

class CANBusScanner {
    private:
        std::unique_ptr<USBAdapter> adapter_;

    public:
        CANBusScanner(const std::string& device) {
            std::cout << "=== CAN Bus Scanner and Diagnostic Tool ===" << std::endl;
            std::cout << "Device: " << device << std::endl << std::endl;

            try {
                adapter_ = USBAdapter::create(device, SerialBaud::BAUD_2M);
                std::cout << "✓ Connected to USB-CAN adapter successfully" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "❌ Failed to connect to CAN adapter: " << e.what() << std::endl;
                throw;
            }
        }

        bool configure_can_bus(CANBaud baud_rate) {
            try {
                std::cout << "Configuring CAN bus at ";
                switch (baud_rate) {
                case CANBaud::BAUD_125K: std::cout << "125kbps"; break;
                case CANBaud::BAUD_250K: std::cout << "250kbps"; break;
                case CANBaud::BAUD_500K: std::cout << "500kbps"; break;
                case CANBaud::BAUD_1M: std::cout << "1Mbps"; break;
                default: std::cout << "unknown"; break;
                }
                std::cout << "..." << std::endl;

                auto config_frame = FrameBuilder<ConfigFrame>()
                    .with_can_version(CANVersion::STD_FIXED)
                    .with_baud_rate(baud_rate)
                    .with_mode(CANMode::NORMAL)
                    .with_rtx(RTX::AUTO)
                    .with_filter(0x000)
                    .with_mask(0x000)
                    .build();

                adapter_->send_frame(config_frame);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                std::cout << "✓ CAN bus configured" << std::endl;
                return true;
            } catch (const std::exception& e) {
                std::cout << "❌ Configuration failed: " << e.what() << std::endl;
                return false;
            }
        }

        void send_node_guard_request(uint8_t node_id) {
            try {
                // Send Node Guard request (NMT protocol)
                auto frame = FrameBuilder<VariableFrame>()
                    .with_type(CANVersion::STD_VARIABLE, Format::REMOTE_VARIABLE)
                    .with_id(0x700 + node_id) // Node Guard COB-ID
                    .build();

                adapter_->send_frame(frame);
            } catch (const std::exception& e) {
                // Silent failure for scanning
            }
        }

        void send_sdo_read_request(uint8_t node_id, uint16_t index, uint8_t subindex = 0) {
            try {
                auto frame = FrameBuilder<VariableFrame>()
                    .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
                    .with_id(0x600 + node_id)
                    .with_data({
                0x40,      // SDO read request
                static_cast<uint8_t>(index & 0xFF),
                static_cast<uint8_t>((index >> 8) & 0xFF),
                subindex,
                0x00, 0x00, 0x00, 0x00
            })
                    .build();

                adapter_->send_frame(frame);
            } catch (const std::exception& e) {
                // Silent failure for scanning
            }
        }

        void listen_for_responses(int duration_ms) {
            std::cout << "Listening for responses (" << duration_ms << "ms)..." << std::endl;

            auto start_time = std::chrono::steady_clock::now();
            int frame_count = 0;
            std::set<uint32_t> seen_ids;

            while (std::chrono::steady_clock::now() - start_time <
                std::chrono::milliseconds(duration_ms)) {
                try {
                    auto frame = adapter_->receive_variable_frame(50); // 50ms timeout
                    frame_count++;

                    uint32_t can_id = frame.get_can_id();
                    if (seen_ids.find(can_id) == seen_ids.end()) {
                        seen_ids.insert(can_id);

                        std::cout << "  📡 Frame from ID 0x" << std::hex << std::uppercase
                                  << std::setfill('0') << std::setw(3) << can_id << std::dec
                                  << " (DLC=" << static_cast<int>(frame.get_dlc()) << ")";

                        // Decode potential node ID and frame type
                        if (can_id >= 0x580 && can_id <= 0x5FF) {
                            std::cout << " - SDO Response from Node " << (can_id - 0x580);
                        } else if (can_id >= 0x700 && can_id <= 0x77F) {
                            std::cout << " - Node Guard from Node " << (can_id - 0x700);
                        } else if (can_id >= 0x180 && can_id <= 0x1FF) {
                            std::cout << " - PDO1 Tx from Node " << (can_id - 0x180);
                        } else if (can_id >= 0x200 && can_id <= 0x27F) {
                            std::cout << " - PDO1 Rx to Node " << (can_id - 0x200);
                        } else if (can_id >= 0x280 && can_id <= 0x2FF) {
                            std::cout << " - PDO2 Tx from Node " << (can_id - 0x280);
                        } else if (can_id >= 0x300 && can_id <= 0x37F) {
                            std::cout << " - PDO2 Rx to Node " << (can_id - 0x300);
                        } else if (can_id >= 0x380 && can_id <= 0x3FF) {
                            std::cout << " - PDO3 Tx from Node " << (can_id - 0x380);
                        } else if (can_id >= 0x400 && can_id <= 0x47F) {
                            std::cout << " - PDO3 Rx to Node " << (can_id - 0x400);
                        } else if (can_id >= 0x480 && can_id <= 0x4FF) {
                            std::cout << " - PDO4 Tx from Node " << (can_id - 0x480);
                        } else if (can_id >= 0x500 && can_id <= 0x57F) {
                            std::cout << " - PDO4 Rx to Node " << (can_id - 0x500);
                        } else if (can_id == 0x000) {
                            std::cout << " - NMT Master";
                        } else if (can_id == 0x080) {
                            std::cout << " - SYNC";
                        } else if (can_id == 0x100) {
                            std::cout << " - TIME";
                        } else {
                            std::cout << " - Unknown/Custom";
                        }

                        // Show data if available
                        if (frame.get_dlc() > 0) {
                            std::cout << " Data: [";
                            for (size_t i = 0; i < frame.get_dlc(); i++) {
                                if (i > 0) std::cout << " ";
                                std::cout << std::hex << std::setfill('0') << std::setw(2)
                                          << static_cast<int>(frame.get_data()[i]);
                            }
                            std::cout << "]" << std::dec;
                        }

                        std::cout << std::endl;
                    }
                } catch (const TimeoutException&) {
                    // Continue listening
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                catch (const std::exception& e) {
                    std::cout << "  ⚠ Error receiving: " << e.what() << std::endl;
                    break;
                }
            }

            std::cout << "Total frames received: " << frame_count << std::endl;
            std::cout << "Unique IDs seen: " << seen_ids.size() << std::endl;
        }

        void scan_for_nodes(CANBaud baud_rate) {
            std::cout << "\n=== Scanning for CANOpen nodes ===" << std::endl;

            if (!configure_can_bus(baud_rate)) {
                return;
            }

            // Method 1: Send Node Guard requests to common node IDs
            std::cout << "\n1. Sending Node Guard requests..." << std::endl;
            for (uint8_t node_id = 1; node_id <= 8; node_id++) {
                send_node_guard_request(node_id);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            listen_for_responses(2000);

            // Method 2: Send SDO requests to Device Type object
            std::cout << "\n2. Sending SDO Device Type requests..." << std::endl;
            for (uint8_t node_id = 1; node_id <= 8; node_id++) {
                send_sdo_read_request(node_id, 0x1000, 0); // Device Type
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            listen_for_responses(2000);

            // Method 3: Just listen for any spontaneous traffic
            std::cout << "\n3. Listening for spontaneous CAN traffic..." << std::endl;
            listen_for_responses(3000);
        }

        void test_loopback() {
            std::cout << "\n=== Testing Adapter Loopback ===" << std::endl;

            try {
                // Configure in loopback mode
                auto config_frame = FrameBuilder<ConfigFrame>()
                    .with_can_version(CANVersion::STD_FIXED)
                    .with_baud_rate(CANBaud::BAUD_500K)
                    .with_mode(CANMode::LOOPBACK)
                    .with_rtx(RTX::AUTO)
                    .with_filter(0x000)
                    .with_mask(0x000)
                    .build();

                adapter_->send_frame(config_frame);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                std::cout << "Configured in loopback mode" << std::endl;

                // Send a test frame
                auto test_frame = FrameBuilder<VariableFrame>()
                    .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
                    .with_id(0x123)
                    .with_data({0xAA, 0xBB, 0xCC, 0xDD})
                    .build();

                std::cout << "Sending test frame..." << std::endl;
                adapter_->send_frame(test_frame);

                // Try to receive it back
                try {
                    auto received = adapter_->receive_variable_frame(1000);
                    std::cout << "✓ Loopback test PASSED - received frame ID 0x"
                              << std::hex << received.get_can_id() << std::dec << std::endl;
                } catch (const TimeoutException&) {
                    std::cout << "❌ Loopback test FAILED - no frame received" << std::endl;
                }

            } catch (const std::exception& e) {
                std::cout << "❌ Loopback test failed: " << e.what() << std::endl;
            }
        }

        void run_full_scan() {
            // Test adapter functionality first
            test_loopback();

            // Test common CAN bus speeds
            std::vector<CANBaud> baud_rates = {
                CANBaud::BAUD_125K,
                CANBaud::BAUD_250K,
                CANBaud::BAUD_500K,
                CANBaud::BAUD_1M
            };

            for (auto baud : baud_rates) {
                scan_for_nodes(baud);
            }

            std::cout << "\n=== Scan Complete ===" << std::endl;
            std::cout << "If no devices were found:" << std::endl;
            std::cout << "1. Check physical CAN bus connections (CAN_H, CAN_L)" << std::endl;
            std::cout << "2. Verify 120Ω termination resistors at both ends" << std::endl;
            std::cout << "3. Ensure devices are powered and configured" << std::endl;
            std::cout << "4. Check if devices use different baud rates" << std::endl;
            std::cout << "5. Verify node IDs are in range 1-127" << std::endl;
        }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <device>" << std::endl;
        std::cout << "Example: " << argv[0] << " /dev/ttyUSB0" << std::endl;
        std::cout << std::endl;
        std::cout << "This tool scans for CANOpen devices on the CAN bus" << std::endl;
        std::cout << "and tests different baud rates and node IDs." << std::endl;
        return 1;
    }

    try {
        CANBusScanner scanner(argv[1]);
        scanner.run_full_scan();

    } catch (const std::exception& e) {
        std::cerr << "❌ Scanner failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
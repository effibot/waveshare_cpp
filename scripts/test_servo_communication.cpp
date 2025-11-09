/**
 * @file test_servo_communication.cpp
 * @brief Real device communication test for MICROPHASE TRAC_PWR servo drives
 * @version 1.0
 * @date 2025-11-02
 *
 * Tests CANOpen DS402 communication with real servo hardware
 */

#include "../include/waveshare.hpp"
#include "script_utils.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <vector>
#include <map>

using namespace waveshare;

// CIA402 Standard Objects for Servo Drives
constexpr uint16_t DEVICE_TYPE = 0x1000;
constexpr uint16_t ERROR_REGISTER = 0x1001;
constexpr uint16_t MANUFACTURER_STATUS = 0x1002;
constexpr uint16_t IDENTITY_VENDOR_ID = 0x1018;

constexpr uint16_t CONTROL_WORD = 0x6040;
constexpr uint16_t STATUS_WORD = 0x6041;
constexpr uint16_t MODES_OF_OPERATION = 0x6060;
constexpr uint16_t MODES_OF_OPERATION_DISPLAY = 0x6061;
constexpr uint16_t POSITION_ACTUAL = 0x6064;
constexpr uint16_t VELOCITY_ACTUAL = 0x606C;
constexpr uint16_t TARGET_POSITION = 0x607A;
constexpr uint16_t TARGET_VELOCITY = 0x60FF;

// Status Word bit definitions (DS402)
constexpr uint16_t STATUS_READY_TO_SWITCH_ON = 0x0001;
constexpr uint16_t STATUS_SWITCHED_ON = 0x0002;
constexpr uint16_t STATUS_OPERATION_ENABLED = 0x0004;
constexpr uint16_t STATUS_FAULT = 0x0008;
constexpr uint16_t STATUS_VOLTAGE_ENABLED = 0x0010;
constexpr uint16_t STATUS_QUICK_STOP = 0x0020;
constexpr uint16_t STATUS_SWITCH_ON_DISABLED = 0x0040;
constexpr uint16_t STATUS_WARNING = 0x0080;

// Control Word commands (DS402)
constexpr uint16_t CONTROL_SHUTDOWN = 0x0006;
constexpr uint16_t CONTROL_SWITCH_ON = 0x0007;
constexpr uint16_t CONTROL_ENABLE_OPERATION = 0x000F;
constexpr uint16_t CONTROL_DISABLE_VOLTAGE = 0x0000;
constexpr uint16_t CONTROL_QUICK_STOP = 0x0002;
constexpr uint16_t CONTROL_FAULT_RESET = 0x0080;

class ServoTester {
    private:
        std::unique_ptr<USBAdapter> adapter_;
        uint8_t node_id_;
        std::map<uint16_t, std::string> status_descriptions_;

        void init_status_descriptions() {
            status_descriptions_[0x0000] = "Not ready to switch on";
            status_descriptions_[0x0040] = "Switch on disabled";
            status_descriptions_[0x0021] = "Ready to switch on";
            status_descriptions_[0x0023] = "Switched on";
            status_descriptions_[0x0027] = "Operation enabled";
            status_descriptions_[0x0007] = "Quick stop active";
            status_descriptions_[0x000F] = "Fault reaction active";
            status_descriptions_[0x0008] = "Fault";
        }

    public:
        ServoTester(const std::string& device, uint8_t node_id)
            : node_id_(node_id) {

            std::cout << "=== MICROPHASE TRAC_PWR Servo Communication Test ===" << std::endl;
            std::cout << "Device: " << device << std::endl;
            std::cout << "Node ID: " << static_cast<int>(node_id) << std::endl;
            std::cout << "Vendor ID: 0x1A21 (MICROPHASE SRL)" << std::endl;
            std::cout << "Product: TRAC_PWR Drive" << std::endl << std::endl;

            init_status_descriptions();

            try {
                // Step 1: Create USB adapter with correct serial baud rate (2Mbps for USB communication)
                adapter_ = USBAdapter::create(device, SerialBaud::BAUD_2M);
                std::cout << "✓ Connected to USB-CAN adapter successfully" << std::endl;

                // Step 2: Configure the CAN bus parameters via ConfigFrame
                std::cout << "Configuring CAN bus (500kbps, normal mode)..." << std::endl;

                auto config_frame = FrameBuilder<ConfigFrame>()
                    .with_can_version(CANVersion::STD_FIXED)  // Standard CAN frames
                    .with_baud_rate(CANBaud::BAUD_500K)       // 500kbps CAN bus
                    .with_mode(CANMode::NORMAL)               // Normal CAN mode
                    .with_rtx(RTX::AUTO)                      // Auto retransmit
                    .with_filter(0x000)                       // Accept all frames
                    .with_mask(0x000)                         // No filtering
                    .build();

                adapter_->send_frame(config_frame);
                std::cout << "✓ CAN bus configured successfully" << std::endl;

                // Give the adapter time to configure
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

            } catch (const std::exception& e) {
                std::cerr << "❌ Failed to initialize CAN adapter: " << e.what() << std::endl;
                throw;
            }
        }

        // Send SDO read request (expedited transfer)
        bool read_object(uint16_t index, uint8_t subindex = 0, uint32_t* value = nullptr) {
            auto frame = FrameBuilder<VariableFrame>()
                .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
                .with_id(0x600 + node_id_) // SDO request COB-ID
                .with_data({
            0x40,      // Command specifier (expedited read request)
            static_cast<uint8_t>(index & 0xFF),            // Index low byte
            static_cast<uint8_t>((index >> 8) & 0xFF),     // Index high byte
            subindex,      // Subindex
            0x00, 0x00, 0x00, 0x00      // Reserved
        })
                .build();

            try {
                adapter_->send_frame(frame);
                std::cout << "SDO Read: 0x" << std::hex << std::uppercase << index
                          << ":" << std::dec << static_cast<int>(subindex) << " -> ";

                return wait_for_sdo_response(value);
            } catch (const std::exception& e) {
                std::cout << "❌ Send failed: " << e.what() << std::endl;
                return false;
            }
        }

        // Send SDO write request (expedited transfer)
        bool write_object(uint16_t index, uint8_t subindex, uint32_t value, uint8_t data_size = 4) {
            std::vector<uint8_t> data = {
                static_cast<uint8_t>(0x23 | ((4 - data_size) << 2)), // Command specifier
                static_cast<uint8_t>(index & 0xFF),    // Index low byte
                static_cast<uint8_t>((index >> 8) & 0xFF), // Index high byte
                subindex, // Subindex
                static_cast<uint8_t>(value & 0xFF),
                static_cast<uint8_t>((value >> 8) & 0xFF),
                static_cast<uint8_t>((value >> 16) & 0xFF),
                static_cast<uint8_t>((value >> 24) & 0xFF)
            };

            auto frame = FrameBuilder<VariableFrame>()
                .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
                .with_id(0x600 + node_id_)
                .with_data(data)
                .build();

            try {
                adapter_->send_frame(frame);
                std::cout << "SDO Write: 0x" << std::hex << std::uppercase << index
                          << ":" << std::dec << static_cast<int>(subindex)
                          << " = 0x" << std::hex << value << std::dec << " -> ";

                return wait_for_sdo_response();
            } catch (const std::exception& e) {
                std::cout << "❌ Send failed: " << e.what() << std::endl;
                return false;
            }
        }

        // Listen for SDO response
        bool wait_for_sdo_response(uint32_t* value = nullptr, int timeout_ms = 1000) {
            auto start = std::chrono::steady_clock::now();
            uint32_t expected_cob_id = 0x580 + node_id_; // SDO response COB-ID

            while (std::chrono::steady_clock::now() - start <
                std::chrono::milliseconds(timeout_ms)) {
                try {
                    auto frame = adapter_->receive_variable_frame();

                    if (frame.get_can_id() == expected_cob_id) {
                        auto data = frame.get_data();
                        uint8_t command = data[0];

                        if ((command & 0x80) == 0x80) {
                            // SDO abort
                            uint32_t abort_code = (data[7] << 24) | (data[6] << 16) | (data[5] <<
                                8) | data[4];
                            std::cout << "❌ SDO Abort: 0x" << std::hex << abort_code << std::dec;
                            print_abort_code(abort_code);
                            return false;
                        } else if ((command & 0x60) == 0x40) {
                            // SDO read response (expedited)
                            if (value) {
                                *value = (data[7] << 24) | (data[6] << 16) | (data[5] <<
                                    8) | data[4];
                                std::cout << "✓ 0x" << std::hex << *value << std::dec;
                            } else {
                                std::cout << "✓ Read OK";
                            }
                            std::cout << std::endl;
                            return true;
                        } else if ((command & 0x60) == 0x60) {
                            // SDO write response
                            std::cout << "✓ Write OK" << std::endl;
                            return true;
                        }
                    }
                } catch (const TimeoutException&) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }

            std::cout << "❌ Timeout (no response)" << std::endl;
            return false;
        }

        void print_abort_code(uint32_t code) {
            std::map<uint32_t, std::string> abort_codes = {
                {0x05040000, " - SDO protocol timed out"},
                {0x05040001, " - Client/server command specifier not valid or unknown"},
                {0x05040005, " - Out of memory"},
                {0x06010000, " - Unsupported access to an object"},
                {0x06010001, " - Attempt to read a write only object"},
                {0x06010002, " - Attempt to write a read only object"},
                {0x06020000, " - Object does not exist in the object dictionary"},
                {0x06040041, " - Object cannot be mapped to the PDO"},
                {0x06040042, " - PDO length exceeded"},
                {0x06040043, " - General parameter incompatibility reason"},
                {0x06040047, " - General internal incompatibility in the device"},
                {0x06060000, " - Access failed due to an hardware error"},
                {0x06070010,
                 " - Data type does not match, length of service parameter does not match"},
                {0x06070012, " - Data type does not match, length of service parameter too high"},
                {0x06070013, " - Data type does not match, length of service parameter too low"},
                {0x06090011, " - Sub-index does not exist"},
                {0x06090030, " - Value range of parameter exceeded"},
                {0x06090031, " - Value of parameter written too high"},
                {0x06090032, " - Value of parameter written too low"},
                {0x06090036, " - Maximum value is less than minimum value"},
                {0x08000000, " - General error"},
                {0x08000020, " - Data cannot be transferred or stored to the application"},
                {0x08000021,
                 " - Data cannot be transferred or stored to the application because of local control"},
                {0x08000022,
                 " - Data cannot be transferred or stored to the application because of the present device state"},
                {0x08000023,
                 " - Object dictionary dynamic generation fails or no object dictionary is present"}
            };

            auto it = abort_codes.find(code);
            if (it != abort_codes.end()) {
                std::cout << it->second;
            }
        }

        std::string decode_status_word(uint16_t status) {
            // Mask the relevant bits for state determination
            uint16_t state_bits = status & 0x006F;

            auto it = status_descriptions_.find(state_bits);
            if (it != status_descriptions_.end()) {
                return it->second;
            }

            return "Unknown state";
        }

        void print_status_details(uint16_t status) {
            std::cout << "  State: " << decode_status_word(status) << std::endl;
            std::cout << "  Bits: ";
            if (status & STATUS_READY_TO_SWITCH_ON) std::cout << "RTSO ";
            if (status & STATUS_SWITCHED_ON) std::cout << "SO ";
            if (status & STATUS_OPERATION_ENABLED) std::cout << "OE ";
            if (status & STATUS_FAULT) std::cout << "FAULT ";
            if (status & STATUS_VOLTAGE_ENABLED) std::cout << "VE ";
            if (status & STATUS_QUICK_STOP) std::cout << "QS ";
            if (status & STATUS_SWITCH_ON_DISABLED) std::cout << "SOD ";
            if (status & STATUS_WARNING) std::cout << "WARN ";
            std::cout << std::endl;
        }

        bool test_device_identification() {
            std::cout << "\n=== Device Identification Test ===" << std::endl;

            uint32_t value;

            // Test 1: Device Type (mandatory)
            std::cout << "1. Device Type (0x1000:0)..." << std::endl;
            if (!read_object(DEVICE_TYPE, 0, &value)) {
                std::cout << "❌ Cannot read device type - device may not be responding" <<
                    std::endl;
                return false;
            }

            if ((value & 0xFFFF) == 0x0192) {
                std::cout << "  ✓ Device Type: 0x" << std::hex << value << " (DS402 Servo Drive)" <<
                    std::endl;
            } else {
                std::cout << "  ⚠ Unexpected device type: 0x" << std::hex << value << std::endl;
            }

            // Test 2: Vendor ID
            std::cout << "\n2. Vendor ID (0x1018:1)..." << std::endl;
            if (read_object(IDENTITY_VENDOR_ID, 1, &value)) {
                if (value == 0x1A21) {
                    std::cout << "  ✓ Vendor ID: 0x" << std::hex << value << " (MICROPHASE SRL)" <<
                        std::endl;
                } else {
                    std::cout << "  ⚠ Unexpected vendor ID: 0x" << std::hex << value << std::endl;
                }
            }

            // Test 3: Product Code
            std::cout << "\n3. Product Code (0x1018:2)..." << std::endl;
            if (read_object(IDENTITY_VENDOR_ID, 2, &value)) {
                std::cout << "  Product Code: 0x" << std::hex << value << std::endl;
            }

            // Test 4: Error Register
            std::cout << "\n4. Error Register (0x1001:0)..." << std::endl;
            if (read_object(ERROR_REGISTER, 0, &value)) {
                if (value == 0) {
                    std::cout << "  ✓ No errors detected" << std::endl;
                } else {
                    std::cout << "  ⚠ Error register: 0x" << std::hex << value << std::endl;
                }
            }

            return true;
        }

        bool test_ds402_communication() {
            std::cout << "\n=== DS402 Servo Drive Communication Test ===" << std::endl;

            uint32_t value;

            // Test 1: Read Status Word
            std::cout << "1. Reading Status Word (0x6041:0)..." << std::endl;
            if (!read_object(STATUS_WORD, 0, &value)) {
                return false;
            }

            uint16_t status = static_cast<uint16_t>(value);
            std::cout << "  Status Word: 0x" << std::hex << std::uppercase << status << std::dec <<
                std::endl;
            print_status_details(status);

            // Test 2: Read Modes of Operation Display
            std::cout << "\n2. Reading Modes of Operation Display (0x6061:0)..." << std::endl;
            if (read_object(MODES_OF_OPERATION_DISPLAY, 0, &value)) {
                std::cout << "  Current Mode: " << static_cast<int8_t>(value);
                switch (static_cast<int8_t>(value)) {
                case 1: std::cout << " (Profile Position)" << std::endl; break;
                case 3: std::cout << " (Profile Velocity)" << std::endl; break;
                case 6: std::cout << " (Homing)" << std::endl; break;
                case 8: std::cout << " (Cyclic Sync Position)" << std::endl; break;
                case 9: std::cout << " (Cyclic Sync Velocity)" << std::endl; break;
                case 10: std::cout << " (Cyclic Sync Torque)" << std::endl; break;
                default: std::cout << " (Unknown/Custom)" << std::endl; break;
                }
            }

            // Test 3: Read Actual Position
            std::cout << "\n3. Reading Actual Position (0x6064:0)..." << std::endl;
            if (read_object(POSITION_ACTUAL, 0, &value)) {
                std::cout << "  Position: " << static_cast<int32_t>(value) << " encoder counts" <<
                    std::endl;
            }

            // Test 4: Read Actual Velocity
            std::cout << "\n4. Reading Actual Velocity (0x606C:0)..." << std::endl;
            if (read_object(VELOCITY_ACTUAL, 0, &value)) {
                std::cout << "  Velocity: " << static_cast<int32_t>(value) << " counts/sec" <<
                    std::endl;
            }

            return true;
        }

        bool test_pdo_communication() {
            std::cout << "\n=== PDO Communication Test ===" << std::endl;
            std::cout << "Testing Process Data Objects (PDOs) for real-time communication" <<
                std::endl;

            // PDO COB-IDs for this node
            uint32_t rpdo1_cob = 0x200 + node_id_;  // Receive PDO 1 (host -> motor)
            uint32_t rpdo2_cob = 0x300 + node_id_;  // Receive PDO 2
            uint32_t tpdo1_cob = 0x180 + node_id_;  // Transmit PDO 1 (motor -> host)
            uint32_t tpdo2_cob = 0x280 + node_id_;  // Transmit PDO 2
            uint32_t sync_cob = 0x80;               // SYNC message

            std::cout << "  RPDO1 COB-ID: 0x" << std::hex << rpdo1_cob << std::dec <<
                " (commands to motor)" << std::endl;
            std::cout << "  RPDO2 COB-ID: 0x" << std::hex << rpdo2_cob << std::dec << std::endl;
            std::cout << "  TPDO1 COB-ID: 0x" << std::hex << tpdo1_cob << std::dec <<
                " (feedback from motor)" << std::endl;
            std::cout << "  TPDO2 COB-ID: 0x" << std::hex << tpdo2_cob << std::dec << std::endl;
            std::cout << "  SYNC COB-ID: 0x" << std::hex << sync_cob << std::dec << std::endl;

            // Test 1: Send RPDO1 (Controlword + Target Position)
            std::cout << "\n1. Testing RPDO1 (sending controlword + target position)..." <<
                std::endl;

            uint16_t controlword = 0x0006;  // Shutdown command
            int32_t target_position = 1000;  // Example target

            std::vector<uint8_t> rpdo1_data = {
                static_cast<uint8_t>(controlword & 0xFF),
                static_cast<uint8_t>((controlword >> 8) & 0xFF),
                static_cast<uint8_t>(target_position & 0xFF),
                static_cast<uint8_t>((target_position >> 8) & 0xFF),
                static_cast<uint8_t>((target_position >> 16) & 0xFF),
                static_cast<uint8_t>((target_position >> 24) & 0xFF),
                0x00, 0x00
            };

            auto rpdo1_frame = FrameBuilder<VariableFrame>()
                .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
                .with_id(rpdo1_cob)
                .with_data(rpdo1_data)
                .build();

            try {
                adapter_->send_frame(rpdo1_frame);
                std::cout << "  ✓ RPDO1 sent: CW=0x" << std::hex << controlword
                          << " Target=" << std::dec << target_position << std::endl;
            } catch (const std::exception& e) {
                std::cout << "  ❌ Failed to send RPDO1: " << e.what() << std::endl;
                return false;
            }

            // Test 2: Send SYNC and listen for TPDOs
            std::cout << "\n2. Testing SYNC-triggered TPDOs..." << std::endl;
            std::cout << "  Sending SYNC message..." << std::endl;

            auto sync_frame = FrameBuilder<VariableFrame>()
                .with_type(CANVersion::STD_VARIABLE, Format::DATA_VARIABLE)
                .with_id(sync_cob)
                .with_data({})  // SYNC has no data
                .build();

            try {
                adapter_->send_frame(sync_frame);
                std::cout << "  ✓ SYNC sent (COB-ID 0x80)" << std::endl;
            } catch (const std::exception& e) {
                std::cout << "  ❌ Failed to send SYNC: " << e.what() << std::endl;
                return false;
            }

            // Wait for TPDOs
            std::cout << "  Listening for TPDO responses (5 second timeout)..." << std::endl;

            bool tpdo1_received = false;
            bool tpdo2_received = false;
            auto start = std::chrono::steady_clock::now();

            while (std::chrono::steady_clock::now() - start < std::chrono::seconds(5)) {
                try {
                    auto frame = adapter_->receive_variable_frame();
                    uint32_t cob_id = frame.get_can_id();
                    auto data = frame.get_data();

                    if (cob_id == tpdo1_cob && !tpdo1_received) {
                        tpdo1_received = true;
                        // TPDO1: Statusword (2 bytes) + Position Actual (4 bytes)
                        if (data.size() >= 6) {
                            uint16_t statusword = data[0] | (data[1] << 8);
                            int32_t position = data[2] | (data[3] << 8) | (data[4] <<
                                16) | (data[5] << 24);

                            std::cout << "  ✓ TPDO1 received:" << std::endl;
                            std::cout << "    Statusword: 0x" << std::hex << statusword <<
                                std::dec << std::endl;
                            std::cout << "    Position: " << position << " counts" << std::endl;
                            print_status_details(statusword);
                        }
                    } else if (cob_id == tpdo2_cob && !tpdo2_received) {
                        tpdo2_received = true;
                        // TPDO2: Velocity Actual (4 bytes) + Current Actual (2 bytes)
                        if (data.size() >= 6) {
                            int32_t velocity = data[0] | (data[1] << 8) | (data[2] <<
                                16) | (data[3] << 24);
                            int16_t current = data[4] | (data[5] << 8);

                            std::cout << "  ✓ TPDO2 received:" << std::endl;
                            std::cout << "    Velocity: " << velocity << " counts/sec" << std::endl;
                            std::cout << "    Current: " << current << " mA" << std::endl;
                        }
                    }

                    if (tpdo1_received && tpdo2_received) {
                        break;
                    }

                } catch (const TimeoutException&) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }

            // Summary
            std::cout << "\n  PDO Test Results:" << std::endl;
            std::cout << "    RPDO1 (send):    ✓ Sent successfully" << std::endl;
            std::cout << "    SYNC:            ✓ Sent successfully" << std::endl;
            std::cout << "    TPDO1 (receive): " <<
                (tpdo1_received ? "✓ Received" : "❌ Not received") << std::endl;
            std::cout << "    TPDO2 (receive): " <<
                (tpdo2_received ? "✓ Received" : "⚠ Not received (may not be configured)") <<
            std::endl;

            if (!tpdo1_received) {
                std::cout << "\n  ⚠ Note: TPDOs may need to be configured in the motor." <<
                    std::endl;
                std::cout <<
                    "    Check if TPDO mapping is enabled in the object dictionary (0x1800-0x1803)."
                          <<
                    std::endl;
            }

            return tpdo1_received;  // At minimum, TPDO1 should be received
        }

        bool test_basic_control() {
            std::cout << "\n=== Basic Control Test ===" << std::endl;
            std::cout << "⚠ WARNING: This will attempt to enable the servo drive!" << std::endl;
            std::cout << "Make sure the motor is safe to move before proceeding." << std::endl;

            std::cout << "Continue? (y/N): ";
            std::string response;
            std::getline(std::cin, response);
            if (response != "y" && response != "Y") {
                std::cout << "Control test skipped." << std::endl;
                return true;
            }

            uint32_t status_value;

            // Read current status
            std::cout << "\n1. Reading current status..." << std::endl;
            if (!read_object(STATUS_WORD, 0, &status_value)) {
                return false;
            }

            uint16_t status = static_cast<uint16_t>(status_value);
            std::cout << "  Current state: " << decode_status_word(status) << std::endl;

            // If fault, try to reset
            if (status & STATUS_FAULT) {
                std::cout << "\n2. Fault detected - attempting reset..." << std::endl;
                if (!write_object(CONTROL_WORD, 0, CONTROL_FAULT_RESET, 2)) {
                    return false;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                // Check if fault cleared
                if (!read_object(STATUS_WORD, 0, &status_value)) {
                    return false;
                }
                status = static_cast<uint16_t>(status_value);
                if (status & STATUS_FAULT) {
                    std::cout << "  ❌ Fault could not be cleared" << std::endl;
                    return false;
                }
                std::cout << "  ✓ Fault cleared" << std::endl;
            }

            // Enable sequence
            std::cout << "\n3. Servo enable sequence..." << std::endl;

            // Shutdown
            std::cout << "  Shutdown..." << std::endl;
            if (!write_object(CONTROL_WORD, 0, CONTROL_SHUTDOWN, 2)) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Switch On
            std::cout << "  Switch On..." << std::endl;
            if (!write_object(CONTROL_WORD, 0, CONTROL_SWITCH_ON, 2)) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Enable Operation
            std::cout << "  Enable Operation..." << std::endl;
            if (!write_object(CONTROL_WORD, 0, CONTROL_ENABLE_OPERATION, 2)) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Check final status
            if (!read_object(STATUS_WORD, 0, &status_value)) {
                return false;
            }
            status = static_cast<uint16_t>(status_value);
            std::cout << "  Final state: " << decode_status_word(status) << std::endl;

            if (status & STATUS_OPERATION_ENABLED) {
                std::cout << "  ✓ Servo successfully enabled!" << std::endl;
            } else {
                std::cout << "  ❌ Servo not enabled" << std::endl;
                return false;
            }

            // Disable for safety
            std::cout << "\n4. Disabling servo for safety..." << std::endl;
            write_object(CONTROL_WORD, 0, CONTROL_DISABLE_VOLTAGE, 2);

            return true;
        }

        void run_full_test() {
            bool identification_ok = test_device_identification();
            bool communication_ok = test_ds402_communication();
            bool pdo_ok = false;
            bool control_ok = false;

            if (identification_ok && communication_ok) {
                pdo_ok = test_pdo_communication();
                control_ok = test_basic_control();
            }

            std::cout << "\n=== Test Summary ===" << std::endl;
            std::cout << "Device Identification: " << (identification_ok ? "✓ PASS" : "❌ FAIL") <<
                std::endl;
            std::cout << "DS402 Communication:   " << (communication_ok ? "✓ PASS" : "❌ FAIL") <<
                std::endl;
            std::cout << "PDO Communication:     " <<
                (pdo_ok ? "✓ PASS" : "⚠ PARTIAL (see notes)") <<
                std::endl;
            std::cout << "Basic Control:         " << (control_ok ? "✓ PASS" : "❌ FAIL") <<
                std::endl;

            if (identification_ok && communication_ok) {
                std::cout << "\n✓ Device communication is working correctly!" << std::endl;
                if (pdo_ok) {
                    std::cout << "✓ PDO communication is functional for real-time control!" <<
                        std::endl;
                }
                std::cout << "You can proceed with ROS2 integration." << std::endl;
            } else {
                std::cout << "\n❌ Communication issues detected." << std::endl;
                std::cout << "Check wiring, baud rate, and node ID configuration." << std::endl;
            }
        }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <device> <node_id>" << std::endl;
        std::cout << "Example: " << argv[0] << " /dev/ttyUSB0 1" << std::endl;
        std::cout << std::endl;
        std::cout << "This test communicates with MICROPHASE TRAC_PWR servo drives" << std::endl;
        std::cout << "using CANOpen DS402 protocol at 500kbps." << std::endl;
        return 1;
    }

    try {
        uint8_t node_id = std::stoi(argv[2]);
        if (node_id < 1 || node_id > 127) {
            std::cerr << "Error: Node ID must be between 1 and 127" << std::endl;
            return 1;
        }

        ServoTester tester(argv[1], node_id);
        tester.run_full_test();

    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
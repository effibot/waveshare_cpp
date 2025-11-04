/**
 * @file wave_writer.cpp
 * @author effibot (andrea.efficace1@gmail.com)
 * @brief Enhanced CAN frame writer for Waveshare USB-CAN adapter
 * @version 0.3
 * @date 2025-11-04
 *
 * Supports multiple sending modes:
 * - Single message
 * - Fixed count
 * - Infinite loop
 * - Incrementing CAN ID
 * - Configurable delay between messages
 *
 * Automatically detects if USB device is busy (bridge running) and
 * falls back to SocketCAN mode.
 *
 * @copyright Copyright (c) 2025
 */

#include "script_utils.hpp"
#include <chrono>
#include <thread>
#include <variant>
#include <csignal>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>
#include <cstring>

using namespace waveshare;

// === Mode Selection ===

enum class TransportMode {
    USB_DIRECT,    // Direct USB adapter access
    SOCKETCAN      // Via SocketCAN interface
};

// Global flag for signal handling
static volatile bool running = true;

/**
 * @brief Signal handler for graceful shutdown
 */
void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n[SIGNAL] Received SIGINT (Ctrl+C) - stopping...\n";
        running = false;
    }
}

/**
 * @brief Check if USB device is available (not locked by another process)
 * @param device_path Path to USB device (e.g., /dev/ttyUSB0)
 * @return true if device is available, false if busy
 */
bool is_usb_device_available(const std::string& device_path) {
    // Try to open device with exclusive access flag
    // O_EXCL would be ideal but not supported on TTY devices in all kernels
    int fd = open(device_path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        // Device doesn't exist or permission denied
        return false;
    }

    // Try to acquire exclusive lock using flock
    // This is the proper way to detect if another process has the device locked
    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
        // flock failed - device is locked by another process
        int saved_errno = errno;
        close(fd);
        if (saved_errno == EWOULDBLOCK || saved_errno == EAGAIN) {
            return false;  // Device is locked
        }
        return false;  // Other flock errors also mean unavailable
    }

    // Successfully locked - release and close
    flock(fd, LOCK_UN);
    close(fd);
    return true;
}

/**
 * @brief Open SocketCAN socket
 * @param interface SocketCAN interface name (e.g., vcan0)
 * @return Socket file descriptor, or -1 on error
 */
int open_socketcan(const std::string& interface) {
    int sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sockfd < 0) {
        std::cerr << "[ERROR] Failed to create SocketCAN socket: " << strerror(errno) << "\n";
        return -1;
    }

    struct ifreq ifr;
    std::strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "[ERROR] SocketCAN interface '" << interface << "' not found: "
                  << strerror(errno) << "\n";
        close(sockfd);
        return -1;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[ERROR] Failed to bind to SocketCAN interface: " << strerror(errno) << "\n";
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/**
 * @brief Send CAN frame via SocketCAN
 * @param sockfd SocketCAN file descriptor
 * @param can_id CAN ID
 * @param data Data bytes
 * @param dlc Data length code
 * @param is_extended True for extended ID (29-bit)
 * @return true on success, false on error
 */
bool send_socketcan_frame(int sockfd, std::uint32_t can_id,
    const std::vector<std::uint8_t>& data,
    std::uint8_t dlc, bool is_extended) {
    struct can_frame frame;
    std::memset(&frame, 0, sizeof(frame));

    // Set ID with extended flag if needed
    frame.can_id = can_id;
    if (is_extended) {
        frame.can_id |= CAN_EFF_FLAG;
    }

    // Copy data
    frame.can_dlc = dlc;
    std::memcpy(frame.data, data.data(), std::min(static_cast<size_t>(dlc), data.size()));

    // Send frame
    ssize_t nbytes = write(sockfd, &frame, sizeof(frame));
    if (nbytes != sizeof(frame)) {
        std::cerr << "[ERROR] Failed to send SocketCAN frame: " << strerror(errno) << "\n";
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    try {
        // Install signal handler
        std::signal(SIGINT, signal_handler);

        // Parse command-line arguments
        ScriptConfig config = parse_arguments(argc, argv, ScriptType::WRITER);

        // === Mode Detection ===
        TransportMode mode = TransportMode::USB_DIRECT;
        std::unique_ptr<USBAdapter> adapter;
        int socketcan_fd = -1;

        // Try to initialize USB adapter first
        std::cout << "=== Transport Mode Detection ===\n";
        std::cout << "Checking USB device: " << config.device << "... ";

        if (!is_usb_device_available(config.device)) {
            std::cout << "BUSY\n";
            std::cout << "  Device is locked (bridge may be running)\n";
            std::cout << "  Falling back to SocketCAN mode\n";

            mode = TransportMode::SOCKETCAN;

            // Open SocketCAN interface
            std::cout << "Opening SocketCAN interface: " << config.socketcan_interface << "... ";
            socketcan_fd = open_socketcan(config.socketcan_interface);
            if (socketcan_fd < 0) {
                std::cerr << "FAILED\n";
                std::cerr <<
                    "\n[ERROR] Cannot access USB device (locked) and SocketCAN unavailable\n";
                std::cerr << "  1. Make sure a bridge is running: ./wave_bridge -d "
                          << config.device << " -i " << config.socketcan_interface << "\n";
                std::cerr << "  2. Or stop the bridge to use USB directly\n";
                return 1;
            }
            std::cout << "OK (fd=" << socketcan_fd << ")\n";
        } else {
            std::cout << "AVAILABLE\n";
            std::cout << "  Using direct USB mode\n";

            mode = TransportMode::USB_DIRECT;

            // Try to initialize adapter - may still fail if device becomes locked
            try {
                adapter = initialize_adapter(config);
            } catch (const DeviceException& e) {
                if (e.status() == Status::DBUSY) {
                    // Device became locked between check and initialization
                    std::cout << "\n[WARNING] Device became locked during initialization\n";
                    std::cout << "  Falling back to SocketCAN mode\n";

                    mode = TransportMode::SOCKETCAN;

                    // Open SocketCAN interface
                    std::cout << "Opening SocketCAN interface: " << config.socketcan_interface <<
                        "... ";
                    socketcan_fd = open_socketcan(config.socketcan_interface);
                    if (socketcan_fd < 0) {
                        std::cerr << "FAILED\n";
                        std::cerr <<
                            "\n[ERROR] Cannot access USB device (locked) and SocketCAN unavailable\n";
                        std::cerr << "  1. Make sure a bridge is running: ./wave_bridge -d "
                                  << config.device << " -i " << config.socketcan_interface << "\n";
                        std::cerr << "  2. Or stop the bridge to use USB directly\n";
                        return 1;
                    }
                    std::cout << "OK (fd=" << socketcan_fd << ")\n";
                } else {
                    // Other device error - rethrow
                    throw;
                }
            }
        }

        std::cout << "\n=== CAN Frame Writer ===\n\n";
        std::cout << "Configuration:\n";
        if (mode == TransportMode::USB_DIRECT) {
            std::cout << "  Transport:       USB Direct\n";
            std::cout << "  Device:          " << config.device << "\n";
            std::cout << "  Serial Baud:     " << static_cast<int>(config.serial_baudrate) <<
                " bps\n";
        } else {
            std::cout << "  Transport:       SocketCAN\n";
            std::cout << "  Interface:       " << config.socketcan_interface << "\n";
        }

        std::cout << "  CAN Baud:        " << static_cast<int>(config.can_baudrate) << " bps\n";
        std::cout << "  Frame Type:      " << (config.use_fixed_frames ? "Fixed" : "Variable") <<
            "\n";
        std::cout << "  Start ID:        0x" << std::hex << std::uppercase << std::setfill('0')
                  << std::setw(config.start_id >
            0x7FF ? 8 : 3) << config.start_id << std::dec << "\n";
        std::cout << "  Data:            " << format_can_data(config.message_data.data(),
            config.message_data.size()) << "\n";
        std::cout << "  Message Gap:     " << config.message_gap_ms << " ms\n";
        std::cout << "  Increment ID:    " << (config.increment_id ? "Yes" : "No") << "\n";

        if (config.writer_mode == WriterMode::LOOP || config.message_count == 0) {
            std::cout << "  Mode:            Infinite loop (press Ctrl+C to stop)\n";
        } else {
            std::cout << "  Mode:            Send " << config.message_count << " messages\n";
        }
        std::cout << "\n";

        // Create data frame based on configuration
        std::variant<FixedFrame, VariableFrame> data_frame;
        std::uint32_t current_id = config.start_id;
        std::uint32_t messages_sent = 0;

        // Determine if we should use extended ID format
        bool is_extended = (current_id > 0x7FF);

        if (config.use_fixed_frames) {
            data_frame = make_fixed_frame()
                .with_can_version(is_extended ? CANVersion::EXT_FIXED : CANVersion::STD_FIXED)
                .with_format(Format::DATA_FIXED)
                .with_id(current_id)
                .with_data(config.message_data)
                .build();
        } else {
            data_frame = make_variable_frame()
                .with_type(is_extended ? CANVersion::EXT_VARIABLE : CANVersion::STD_VARIABLE,
                Format::DATA_VARIABLE)
                .with_id(current_id)
                .with_data(config.message_data)
                .build();
        }

        std::cout << "Starting transmission...\n\n";

        // Main sending loop
        while (running) {
            // Check if we've reached the message count (if not in loop mode)
            if (config.writer_mode != WriterMode::LOOP && config.message_count > 0) {
                if (messages_sent >= config.message_count) {
                    break;
                }
            }

            // Send frame based on transport mode
            if (mode == TransportMode::USB_DIRECT) {
                // USB Direct mode - use USBAdapter
                if (std::holds_alternative<FixedFrame>(data_frame)) {
                    std::get<FixedFrame>(data_frame).set_id(current_id);
                    const auto& frame = std::get<FixedFrame>(data_frame);
                    adapter->send_frame(frame);
                    std::cout << "[" << get_timestamp() << "] USB >> " << frame.to_string() << "\n";
                } else if (std::holds_alternative<VariableFrame>(data_frame)) {
                    std::get<VariableFrame>(data_frame).set_id(current_id);
                    const auto& frame = std::get<VariableFrame>(data_frame);
                    adapter->send_frame(frame);
                    std::cout << "[" << get_timestamp() << "] USB >> " << frame.to_string() << "\n";
                }
            } else {
                // SocketCAN mode - use raw socket
                bool success = send_socketcan_frame(
                    socketcan_fd,
                    current_id,
                    config.message_data,
                    static_cast<std::uint8_t>(config.message_data.size()),
                    is_extended
                );

                if (success) {
                    std::cout << "[" << get_timestamp() << "] CAN >> ID: 0x"
                              << std::hex << std::uppercase << std::setfill('0')
                              << std::setw(is_extended ? 8 : 3) << current_id
                              << std::dec << " Data: ["
                              << format_can_data(config.message_data.data(),
                        config.message_data.size())
                              << "]\n";
                } else {
                    std::cerr << "[ERROR] Failed to send frame\n";
                }
            }

            messages_sent++;

            // Increment ID if requested
            if (config.increment_id) {
                current_id++;
                // Wrap around at max ID for standard/extended frames
                if (is_extended && current_id > 0x1FFFFFFF) {
                    current_id = config.start_id;
                } else if (!is_extended && current_id > 0x7FF) {
                    current_id = config.start_id;
                }
            }

            // Wait before sending next frame (if not the last message)
            bool is_last_message = (config.writer_mode != WriterMode::LOOP &&
                config.message_count > 0 && messages_sent >= config.message_count);

            if (!is_last_message && config.message_gap_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(config.message_gap_ms));
            }
        }

        std::cout << "\n=== Transmission Complete ===\n";
        std::cout << "Total messages sent: " << messages_sent << "\n";

        // Cleanup
        if (mode == TransportMode::SOCKETCAN && socketcan_fd >= 0) {
            close(socketcan_fd);
        }

        return 0;
    } catch (const WaveshareException& e) {
        std::cerr << "\n[ERROR] Waveshare error: " << e.what() << "\n";
        std::cerr << "  Status code: " << static_cast<int>(e.status()) << "\n";
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "\n[ERROR] " << e.what() << "\n";
        return 1;
    }
}
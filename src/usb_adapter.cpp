/**
 * @file usb_adapter.cpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief USB-CAN Adapter implementation
 * @version 0.1
 * @date 2025-10-10
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "../include/pattern/usb_adapter.hpp"
#include "../include/io/real_serial_port.hpp"


namespace waveshare {

    // ===================================================================
    // Constructor / Factory
    // ===================================================================

    USBAdapter::USBAdapter(std::unique_ptr<ISerialPort> serial_port, std::string usb_dev,
        SerialBaud baudrate)
        : serial_port_(std::move(serial_port)), usb_device_(std::move(usb_dev)),
        baudrate_(baudrate) {

        // Register the SIGINT handler
        std::signal(SIGINT, sigint_handler);

        if (!serial_port_ || !serial_port_->is_open()) {
            throw DeviceException(Status::DNOT_OPEN, "USBAdapter: serial port not open");
        }

        is_configured_ = true;  // Port is already configured by RealSerialPort
    }

    std::unique_ptr<USBAdapter> USBAdapter::create(const std::string& usb_dev,
        SerialBaud baudrate) {
        // Create real serial port (auto-opens and configures)
        auto serial_port = std::make_unique<RealSerialPort>(usb_dev, baudrate);

        // Create adapter with injected port
        return std::unique_ptr<USBAdapter>(new USBAdapter(std::move(serial_port), usb_dev,
            baudrate));
    }

    // ===================================================================
    // Port management methods (removed - now in RealSerialPort)
    // ===================================================================

    // # Signal handling
    void USBAdapter::sigint_handler(int signum) {
        std::fprintf(stdout, "Received signal %d. Stopping...\n", signum);
        // Set the stop flag to true
        stop_flag = true;
        // Propagate the signal to default handler to terminate the program
        std::signal(signum, SIG_DFL);
        std::raise(signum);
    }

    // === Thread-safe I/O operations ===

    int USBAdapter::write_bytes(const std::uint8_t* data, std::size_t size) {
        if (data == nullptr || size == 0) {
            throw ProtocolException(Status::WBAD_LENGTH, "write_bytes: invalid parameters");
        }

        // Shared lock to check state
        {
            std::shared_lock<std::shared_mutex> state_lock(state_mutex_);
            if (!serial_port_ || !serial_port_->is_open() || !is_configured_) {
                throw DeviceException(Status::DNOT_OPEN, "write_bytes: port not open/configured");
            }
        }

        // Exclusive write lock - prevents concurrent writes
        std::lock_guard<std::mutex> write_lock(write_mutex_);

        ssize_t bytes_written = serial_port_->write(data, size);
        if (bytes_written < 0) {
            throw DeviceException(Status::DWRITE_ERROR,
                "write_bytes: " + std::string(std::strerror(errno)));
        }

        return static_cast<int>(bytes_written);
    }

    int USBAdapter::read_bytes(std::uint8_t* buffer, std::size_t size) {
        if (buffer == nullptr || size == 0) {
            throw ProtocolException(Status::WBAD_LENGTH, "read_bytes: invalid parameters");
        }

        // Shared lock to check state
        {
            std::shared_lock<std::shared_mutex> state_lock(state_mutex_);
            if (!serial_port_ || !serial_port_->is_open() || !is_configured_) {
                throw DeviceException(Status::DNOT_OPEN, "read_bytes: port not open/configured");
            }
        }

        // Exclusive read lock - prevents concurrent reads
        std::lock_guard<std::mutex> read_lock(read_mutex_);

        ssize_t bytes_read = serial_port_->read(buffer, size, -1);  // -1 = use port's default timeout
        if (bytes_read < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Non-blocking read with no data available
                return 0;
            }
            throw DeviceException(Status::DREAD_ERROR,
                "read_bytes: " + std::string(std::strerror(errno)));
        }

        return static_cast<int>(bytes_read);
    }

    void USBAdapter::read_exact(std::uint8_t* buffer, std::size_t size, int timeout_ms) {
        if (buffer == nullptr || size == 0) {
            throw ProtocolException(Status::WBAD_LENGTH, "read_exact: invalid parameters");
        }

        auto start_time = std::chrono::steady_clock::now();
        std::size_t total_read = 0;

        while (total_read < size) {
            // Check timeout
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time
            ).count();

            if (elapsed > timeout_ms) {
                throw TimeoutException(Status::WTIMEOUT,
                    "read_exact: timeout after " + std::to_string(elapsed) + "ms");
            }

            // Read remaining bytes (throws on error)
            int bytes_read = read_bytes(buffer + total_read, size - total_read);
            total_read += bytes_read;
        }
    }



    // === Frame-Level API ===


    FixedFrame USBAdapter::receive_fixed_frame(int timeout_ms) {
        // Read exactly 20 bytes with timeout (throws on timeout/error)
        std::uint8_t buffer[20];
        read_exact(buffer, 20, timeout_ms);

        // State-First: Deserialize buffer into frame state (throws on protocol error)
        FixedFrame frame;
        frame.deserialize(span<const std::uint8_t>(buffer, 20));

        return frame;
    }

    VariableFrame USBAdapter::receive_variable_frame(int timeout_ms) {

        std::vector<std::uint8_t> frame_buffer;
        frame_buffer.reserve(15); // Max VariableFrame size

        auto start_time = std::chrono::steady_clock::now();
        bool found_start = false;

        while (true) {
            // Check timeout
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time
            ).count();

            if (elapsed > timeout_ms) {
                throw TimeoutException(Status::WTIMEOUT,
                    "receive_variable_frame: timeout after " + std::to_string(elapsed) + "ms");
            }

            // Read single byte (throws on error)
            std::uint8_t byte;
            int bytes_read = read_bytes(&byte, 1);

            if (bytes_read == 0) {
                continue; // No data available (non-blocking read)
            }

            // Look for START byte (0xAA)
            if (!found_start) {
                if (byte == 0xAA) {
                    found_start = true;
                    frame_buffer.push_back(byte);
                }
                continue;
            }

            // Accumulate frame bytes
            frame_buffer.push_back(byte);

            // Check for END byte (0x55)
            if (byte == 0x55) {
                // State-First: Deserialize buffer into VariableFrame state (throws on protocol error)
                VariableFrame frame;
                frame.deserialize(
                    span<const std::uint8_t>(frame_buffer.data(), frame_buffer.size())
                );

                return frame;
            }

            // Prevent runaway buffer growth
            if (frame_buffer.size() > 15) {
                throw ProtocolException(Status::WBAD_LENGTH,
                    "receive_variable_frame: Frame too long");
            }
        }
    }

}
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

namespace USBCANBridge {

    // # Port management methods

    Result<void> USBAdapter::open_port() {
        // Exclusive lock for state modification
        std::unique_lock<std::shared_mutex> lock(state_mutex_);

        // If the port is already open, return an error
        if (is_open_) {
            return Result<void>::error(Status::DALREADY_OPEN, "open_port");
        }

        // Open the serial port
        fd_ = open(usb_device_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd_ == -1) {
            // Get errno and return as Status
            return Result<void>::error(Status::DNOT_FOUND, std::strerror(errno));
        }
        is_open_ = true;
        is_configured_ = false; // Port is not configured yet

        // Return success if the port is opened
        return Result<void>::success("Unconfigured port opened successfully");

    }

    Result<void> USBAdapter::configure_port() {
        // Shared lock to check state
        std::shared_lock<std::shared_mutex> read_lock(state_mutex_);

        // If the port is not open, return an error
        if (!is_open_ || fd_ == -1) {
            return Result<void>::error(Status::DNOT_OPEN, "configure_port");
        }

        // Unlock shared, acquire unique for configuration
        read_lock.unlock();
        std::unique_lock<std::shared_mutex> write_lock(state_mutex_);

        // Re-check after acquiring unique lock (double-check pattern)
        if (!is_open_ || fd_ == -1) {
            return Result<void>::error(Status::DNOT_OPEN, "configure_port");
        }

        // Get the current attributes of the Serial port
        if (tcgetattr(fd_, &tty_) != 0) {
            // Get errno and return as Status
            return Result<void>::error(Status::DNOT_OPEN, std::strerror(errno));
        }

        // Initialize the tty_ structure to raw mode
        cfmakeraw(&tty_);

        // Set I/O baud rate
        speed_t io_speed = to_speed_t(get_baudrate());
        cfsetospeed(&tty_, io_speed);
        cfsetispeed(&tty_, io_speed);

        // # Set port settings: 8 bits, no parity, 1 stop bit (8N1)
        tty_.c_cflag &= ~PARENB; // # deactivate generation and check of parity bit
        tty_.c_cflag &= ~CSTOPB; // # 1 stop bit
        tty_.c_cflag &= ~CSIZE;  // # Clear current char size mask
        tty_.c_cflag |= CS8;     // # set the char size to 8 data bits
        tty_.c_cflag |= CREAD | CLOCAL; // # Enable receiver, ignore modem control lines
        tty_.c_cflag |= CRTSCTS; // # Enable hardware flow control
        // ? not sure if needed:
        tty_.c_iflag &= ~(IXON | IXOFF | IXANY); // # Disable software flow control

        // # Set port timeout to 1 second, no minimum characters to read
        tty_.c_cc[VMIN] = 0;    // # Minimum number of characters to read
        tty_.c_cc[VTIME] = 10;  // # Timeout in deciseconds (1 second)


        // Apply the new settings
        if (tcsetattr(fd_, TCSANOW, &tty_) != 0) {
            return Result<void>::error(Status::DNOT_OPEN, std::strerror(errno));
        }

        // Set flag and return success if the port is configured
        is_configured_ = true;
        return Result<void>::success("Port configured successfully");
    }

    Result<void> USBAdapter::close_port() {
        // Exclusive lock for state modification
        std::unique_lock<std::shared_mutex> lock(state_mutex_);

        // If the port is not open, return an error
        if (!is_open_ || fd_ == -1) {
            return Result<void>::error(Status::DNOT_OPEN, "close_port");
        }

        // Close the serial port
        if (close(fd_) == -1) {
            // Get errno and return as Status
            return Result<void>::error(Status::DNOT_OPEN, std::strerror(errno));
        }
        fd_ = -1;
        is_open_ = false;
        is_configured_ = false;

        // Return success if the port is closed
        return Result<void>::success("Port closed successfully");
    }

    // # Signal handling
    void USBAdapter::sigint_handler(int signum) {
        (void)signum; // Unused parameter
        stop_flag = true;
    }

    // === Thread-safe I/O operations ===

    Result<int> USBAdapter::write_bytes(const std::uint8_t* data, std::size_t size) {
        if (data == nullptr || size == 0) {
            return Result<int>::error(Status::WBAD_LENGTH, "write_bytes");
        }

        // Shared lock to check state
        {
            std::shared_lock<std::shared_mutex> state_lock(state_mutex_);
            if (!is_open_ || !is_configured_ || fd_ == -1) {
                return Result<int>::error(Status::DNOT_OPEN, "write_bytes");
            }
        }

        // Exclusive write lock - prevents concurrent writes
        std::lock_guard<std::mutex> write_lock(write_mutex_);

        ssize_t bytes_written = write(fd_, data, size);
        if (bytes_written < 0) {
            return Result<int>::error(Status::DNOT_OPEN, std::strerror(errno));
        }

        return Result<int>::success(static_cast<int>(bytes_written));
    }

    Result<int> USBAdapter::read_bytes(std::uint8_t* buffer, std::size_t size) {
        if (buffer == nullptr || size == 0) {
            return Result<int>::error(Status::WBAD_LENGTH, "read_bytes");
        }

        // Shared lock to check state
        {
            std::shared_lock<std::shared_mutex> state_lock(state_mutex_);
            if (!is_open_ || !is_configured_ || fd_ == -1) {
                return Result<int>::error(Status::DNOT_OPEN, "read_bytes");
            }
        }

        // Exclusive read lock - prevents concurrent reads
        std::lock_guard<std::mutex> read_lock(read_mutex_);

        ssize_t bytes_read = read(fd_, buffer, size);
        if (bytes_read < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Non-blocking read with no data available
                return Result<int>::success(0);
            }
            return Result<int>::error(Status::DNOT_OPEN, std::strerror(errno));
        }

        return Result<int>::success(static_cast<int>(bytes_read));
    }

    Result<void> USBAdapter::read_exact(std::uint8_t* buffer, std::size_t size, int timeout_ms) {
        if (buffer == nullptr || size == 0) {
            return Result<void>::error(Status::WBAD_LENGTH, "read_exact");
        }

        auto start_time = std::chrono::steady_clock::now();
        std::size_t total_read = 0;

        while (total_read < size) {
            // Check timeout
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time
            ).count();

            if (elapsed > timeout_ms) {
                return Result<void>::error(Status::WTIMEOUT, "read_exact");
            }

            // Read remaining bytes
            auto res = read_bytes(buffer + total_read, size - total_read);
            if (res.fail()) {
                return Result<void>::error(res.error(), "read_exact");
            }

            total_read += res.value();
        }

        return Result<void>::success();
    }

    Result<void> USBAdapter::flush_port() {
        // Shared lock to check state
        {
            std::shared_lock<std::shared_mutex> state_lock(state_mutex_);
            if (!is_open_ || fd_ == -1) {
                return Result<void>::error(Status::DNOT_OPEN, "flush_port");
            }
        }

        // Acquire both locks to ensure no concurrent I/O during flush
        std::lock_guard<std::mutex> write_lock(write_mutex_);
        std::lock_guard<std::mutex> read_lock(read_mutex_);

        if (tcflush(fd_, TCIOFLUSH) != 0) {
            return Result<void>::error(Status::DNOT_OPEN, std::strerror(errno));
        }

        return Result<void>::success("Port flushed successfully");
    }

    // === Frame-Level API ===

    template<typename Frame>
    Result<void> USBAdapter::send_frame(const Frame& frame) {
        // State-First: Generate protocol buffer from frame state
        auto buffer = frame.serialize();

        // Write all bytes atomically (thread-safe via write_mutex_)
        auto res = write_bytes(buffer.data(), buffer.size());
        if (res.fail()) {
            return Result<void>::error(res.get_status(), "send_frame");
        }

        // Verify all bytes written
        if (res.get_value() != static_cast<int>(buffer.size())) {
            return Result<void>::error(Status::DNOT_OPEN,
                "send_frame: Partial write " + std::to_string(res.get_value()) +
                "/" + std::to_string(buffer.size()));
        }

        return Result<void>::success();
    }

    template<typename Frame>
    Result<Frame> USBAdapter::receive_fixed_frame(int timeout_ms) {
        static_assert(is_fixed_frame_v<Frame>,
            "Frame type must be FixedFrame");

        // Read exactly 20 bytes with timeout
        std::uint8_t buffer[20];
        auto read_res = read_exact(buffer, 20, timeout_ms);
        if (read_res.fail()) {
            return Result<Frame>::error(read_res, "receive_fixed_frame");
        }

        // State-First: Deserialize buffer into frame state
        Frame frame;
        auto deser_res = frame.deserialize(span<const std::uint8_t>(buffer, 20));
        if (deser_res.fail()) {
            return Result<Frame>::error(deser_res, "receive_fixed_frame");
        }

        return Result<Frame>::success(std::move(frame));
    }

    template<typename Frame>
    Result<Frame> USBAdapter::receive_variable_frame(int timeout_ms) {
        static_assert(!is_fixed_frame_v<Frame>,
            "Frame type must be VariableFrame");
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
                return Result<Frame>::error(Status::WTIMEOUT, "receive_variable_frame");
            }

            // Read single byte
            std::uint8_t byte;
            auto read_res = read_bytes(&byte, 1);

            if (read_res.fail()) {
                return Result<Frame>::error(read_res,
                    "receive_variable_frame");
            }

            if (read_res.value() == 0) {
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
                // State-First: Deserialize buffer into VariableFrame state
                Frame frame;
                auto deser_res = frame.deserialize(
                    span<const std::uint8_t>(frame_buffer.data(), frame_buffer.size())
                );

                if (deser_res.fail()) {
                    return Result<Frame>::error(deser_res,
                        "receive_variable_frame");
                }

                return Result<Frame>::success(std::move(frame));
            }

            // Prevent runaway buffer growth
            if (frame_buffer.size() > 15) {
                return Result<Frame>::error(Status::WBAD_LENGTH,
                    "receive_variable_frame: Frame too long");
            }
        }
    }

}
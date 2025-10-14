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


namespace waveshare {

    // # Port management methods

    void USBAdapter::open_port() {
        // Exclusive lock for state modification
        std::unique_lock<std::shared_mutex> lock(state_mutex_);

        // If the port is already open, throw an error
        if (is_open_) {
            throw DeviceException(Status::DALREADY_OPEN, "open_port");
        }

        // Open the serial port
        fd_ = open(usb_device_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd_ == -1) {
            // Get errno and throw as DeviceException
            throw DeviceException(Status::DNOT_FOUND,
                "open_port: " + std::string(std::strerror(errno)));
        }
        is_open_ = true;
        is_configured_ = false; // Port is not configured yet

        // Log success
        std::fprintf(stdout, "Serial port %s opened successfully.\n", usb_device_.c_str());
    }

    void USBAdapter::configure_port() {
        // Shared lock to check state
        std::shared_lock<std::shared_mutex> read_lock(state_mutex_);

        // If the port is not open, throw an error
        if (!is_open_ || fd_ == -1) {
            throw DeviceException(Status::DNOT_OPEN, "configure_port: port not open");
        }

        // Unlock shared, acquire unique for configuration
        read_lock.unlock();
        std::unique_lock<std::shared_mutex> write_lock(state_mutex_);

        // Re-check after acquiring unique lock (double-check pattern)
        if (!is_open_ || fd_ == -1) {
            throw DeviceException(Status::DNOT_OPEN, "configure_port: port not open");
        }

        // Get current port settings
        int result = ioctl(fd_, TCGETS2, &tty_);
        if (result != 0) {
            throw DeviceException(Status::DNOT_OPEN,
                "configure_port: ioctl TCGETS2 failed: " + std::string(std::strerror(errno)));
        }

        // Convert SerialBaud enum to speed_t
        speed_t baud = to_speed_t(baudrate_);

        // <<< Set port parameters according to Waveshare C example code
        tty_.c_cflag &= ~CBAUD; // # Clear current baud rate bits
        //tty_.c_cflag &= ~CSIZE; // # Clear current character size bits
        tty_.c_cflag = BOTHER // # Use custom baud rate
            | CS8         // # Use payload with 8 data bits
            | CSTOPB;         // # Use 2 stop bits
        tty_.c_iflag = IGNPAR;  // # Ignore framing and parity errors
        tty_.c_oflag = 0;    // # No output processing
        tty_.c_lflag = 0;   // # Non-canonical mode, no echo, no signals
        tty_.c_ispeed = baud;           // # Set input baud rate
        tty_.c_ospeed = baud;   // # Set output baud rate

        // Apply the settings to the port
        result = ioctl(fd_, TCSETS2, &tty_);
        if (result != 0) {
            throw DeviceException(Status::DNOT_OPEN,
                "configure_port: ioctl TCSETS2 failed: " + std::string(std::strerror(errno)));
        }
        // Set flag and log success
        is_configured_ = true;
        std::fprintf(stdout, "Serial port %s configured successfully at %d baud.\n",
            usb_device_.c_str(), static_cast<int>(baudrate_));
    }

    void USBAdapter::close_port() {
        // Exclusive lock for state modification
        std::unique_lock<std::shared_mutex> lock(state_mutex_);

        // If the port is not open, throw an error
        if (!is_open_ || fd_ == -1) {
            throw DeviceException(Status::DNOT_OPEN, "close_port: port not open");
        }

        // Close the serial port
        if (close(fd_) == -1) {
            // Get errno and throw as DeviceException
            throw DeviceException(Status::DNOT_OPEN,
                "close_port: " + std::string(std::strerror(errno)));
        }
        fd_ = -1;
        is_open_ = false;
        is_configured_ = false;

        // Log success
        std::fprintf(stdout, "Serial port %s closed successfully.\n", usb_device_.c_str());
    }

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
            if (!is_open_ || !is_configured_ || fd_ == -1) {
                throw DeviceException(Status::DNOT_OPEN, "write_bytes: port not open/configured");
            }
        }

        // Exclusive write lock - prevents concurrent writes
        std::lock_guard<std::mutex> write_lock(write_mutex_);

        ssize_t bytes_written = write(fd_, data, size);
        if (bytes_written < 0) {
            throw DeviceException(Status::DNOT_OPEN,
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
            if (!is_open_ || !is_configured_ || fd_ == -1) {
                throw DeviceException(Status::DNOT_OPEN, "read_bytes: port not open/configured");
            }
        }

        // Exclusive read lock - prevents concurrent reads
        std::lock_guard<std::mutex> read_lock(read_mutex_);

        ssize_t bytes_read = read(fd_, buffer, size);
        if (bytes_read < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Non-blocking read with no data available
                return 0;
            }
            throw DeviceException(Status::DNOT_OPEN,
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
/**
 * @file real_serial_port.cpp
 * @brief Real serial port implementation
 * @version 1.0
 * @date 2025-10-14
 */

#include "../include/io/real_serial_port.hpp"
#include <cstdio>

namespace waveshare {

    // ===================================================================
    // Constructor / Destructor
    // ===================================================================

    RealSerialPort::RealSerialPort(const std::string& device_path, SerialBaud baud_rate)
        : device_path_(device_path), baud_rate_(baud_rate) {
        open_port();
        configure_port();
    }

    RealSerialPort::~RealSerialPort() {
        if (is_open_ && fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
            is_open_ = false;
        }
    }

    // ===================================================================
    // ISerialPort Implementation
    // ===================================================================

    ssize_t RealSerialPort::write(const void* data, std::size_t len) {
        if (!is_open_ || fd_ < 0) {
            errno = ENOTCONN;
            return -1;
        }

        ssize_t bytes_written = ::write(fd_, data, len);
        return bytes_written;  // Returns -1 on error, errno set by write()
    }

    ssize_t RealSerialPort::read(void* data, std::size_t len, int timeout_ms) {
        if (!is_open_ || fd_ < 0) {
            errno = ENOTCONN;
            return -1;
        }

        // For serial ports, timeout is set via termios VTIME (in deciseconds)
        // For now, we use non-blocking read and let USBAdapter handle timeout
        // This matches the existing USBAdapter behavior

        ssize_t bytes_read = ::read(fd_, data, len);

        // Non-blocking read: return 0 if no data, -1 on error
        if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 0;  // No data available
        }

        return bytes_read;  // Returns -1 on error, errno set by read()
    }

    void RealSerialPort::close() {
        if (is_open_ && fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
            is_open_ = false;
        }
    }

    // ===================================================================
    // Private Methods (Extracted from USBAdapter)
    // ===================================================================

    void RealSerialPort::open_port() {
        // Open the serial port
        fd_ = ::open(device_path_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd_ < 0) {
            throw DeviceException(Status::DNOT_FOUND,
                "RealSerialPort::open_port: " + std::string(std::strerror(errno)));
        }
        is_open_ = true;
        std::fprintf(stdout, "Serial port %s opened successfully.\n", device_path_.c_str());
    }

    void RealSerialPort::configure_port() {
        if (!is_open_ || fd_ < 0) {
            throw DeviceException(Status::DNOT_OPEN,
                "RealSerialPort::configure_port: port not open");
        }

        // Get current port settings
        int result = ::ioctl(fd_, TCGETS2, &tty_);
        if (result != 0) {
            throw DeviceException(Status::DCONFIG_ERROR,
                "RealSerialPort::configure_port: ioctl TCGETS2 failed: " +
                std::string(std::strerror(errno)));
        }

        // Convert SerialBaud enum to speed_t
        speed_t baud = to_speed_t(baud_rate_);

        // Set port parameters according to Waveshare C example code
        tty_.c_cflag &= ~CBAUD;  // Clear current baud rate bits
        tty_.c_cflag = BOTHER    // Use custom baud rate
            | CS8                // 8 data bits
            | CSTOPB;            // 2 stop bits
        tty_.c_iflag = IGNPAR;   // Ignore framing and parity errors
        tty_.c_oflag = 0;        // No output processing
        tty_.c_lflag = 0;        // Non-canonical mode, no echo, no signals
        tty_.c_ispeed = baud;    // Set input baud rate
        tty_.c_ospeed = baud;    // Set output baud rate
        tty_.c_cc[VTIME] = 1;  // 0.1 second timeout
        tty_.c_cc[VMIN] = 0;   // Return immediately if data available

        // Apply the settings to the port
        result = ::ioctl(fd_, TCSETS2, &tty_);
        if (result != 0) {
            throw DeviceException(Status::DCONFIG_ERROR,
                "RealSerialPort::configure_port: ioctl TCSETS2 failed: " +
                std::string(std::strerror(errno)));
        }

        std::fprintf(stdout, "Serial port %s configured successfully at %d baud.\n",
            device_path_.c_str(), static_cast<int>(baud_rate_));
    }

} // namespace waveshare

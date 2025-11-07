/**
 * @file sdo_client.cpp
 * @author effibot (andrea.efficace1@gmail.com)
 * @brief CANopen SDO Client implementation using raw SocketCAN
 * @version 0.1
 * @date 2025-11-06
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "canopen/sdo_client.hpp"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <poll.h>
#include <iostream>
#include <iomanip>

namespace canopen {

    SDOClient::SDOClient(
        const std::string& can_interface,
        const ObjectDictionary& dictionary,
        uint8_t node_id
    ) : dictionary_(dictionary),
        node_id_(node_id),
        socket_fd_(-1),
        can_interface_(can_interface) {
        open_socket();
    }

    SDOClient::~SDOClient() {
        close_socket();
    }

    void SDOClient::open_socket() {
        // Create CAN socket
        socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (socket_fd_ < 0) {
            throw std::runtime_error("Failed to create CAN socket: " +
                std::string(strerror(errno)));
        }

        // Get interface index
        struct ifreq ifr;
        std::strncpy(ifr.ifr_name, can_interface_.c_str(), IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';

        if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
            close(socket_fd_);
            throw std::runtime_error("CAN interface not found: " + can_interface_ +
                " (Error: " + std::string(strerror(errno)) + ")");
        }

        // Bind socket to interface
        struct sockaddr_can addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        if (bind(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(socket_fd_);
            throw std::runtime_error("Failed to bind CAN socket: " + std::string(strerror(errno)));
        }

        std::cout << "[SDO] Connected to " << can_interface_
                  << " (Node ID: " << static_cast<int>(node_id_) << ")\n";
    }

    void SDOClient::close_socket() {
        if (socket_fd_ >= 0) {
            close(socket_fd_);
            socket_fd_ = -1;
        }
    }

    bool SDOClient::write_object(
        const std::string& object_name,
        const std::vector<uint8_t>& data,
        std::chrono::milliseconds timeout
    ) {
        auto obj = dictionary_.get_object(object_name);

        std::cout << "[SDO] Writing " << object_name
                  << " (0x" << std::hex << obj.index << std::dec << "." <<
            static_cast<int>(obj.subindex) << "): ";
        for (auto byte : data) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) <<
                " ";
        }
        std::cout << std::dec << std::endl;

        // Create SDO write frame
        can_frame frame = create_sdo_write_expedited(obj.index, obj.subindex, data);

        // Send request
        if (!send_frame(frame)) {
            std::cerr << "[SDO] Failed to send write request for " << object_name << "\n";
            return false;
        }

        // Wait for response
        can_frame response;
        if (!receive_frame(response, timeout)) {
            std::cerr << "[SDO] Timeout waiting for write confirmation\n";
            return false;
        }

        // Validate response
        if (!validate_sdo_response(response, obj.index, obj.subindex)) {
            std::cerr << "[SDO] Invalid response for " << object_name << "\n";
            return false;
        }

        std::cout << "[SDO] Write successful\n";
        return true;
    }

    std::vector<uint8_t> SDOClient::read_object(
        const std::string& object_name,
        std::chrono::milliseconds timeout
    ) {
        auto obj = dictionary_.get_object(object_name);

        std::cout << "[SDO] Reading " << object_name
                  << " (0x" << std::hex << obj.index << std::dec << "." <<
            static_cast<int>(obj.subindex) << ")\n";

        // Create SDO read request
        can_frame frame = create_sdo_read_request(obj.index, obj.subindex);

        // Send request
        if (!send_frame(frame)) {
            throw std::runtime_error("Failed to send read request for " + object_name);
        }

        // Wait for response
        can_frame response;
        if (!receive_frame(response, timeout)) {
            throw std::runtime_error("Timeout reading " + object_name);
        }

        // Validate and extract data
        if (!validate_sdo_response(response, obj.index, obj.subindex)) {
            throw std::runtime_error("Invalid response for " + object_name);
        }

        // Extract data from response (bytes 4-7 for expedited transfer)
        size_t data_size = obj.size_bytes();
        std::vector<uint8_t> data(response.data + 4, response.data + 4 + data_size);

        std::cout << "[SDO] Read: ";
        for (auto byte : data) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) <<
                " ";
        }
        std::cout << std::dec << std::endl;

        return data;
    }

    can_frame SDOClient::create_sdo_write_expedited(
        uint16_t index,
        uint8_t subindex,
        const std::vector<uint8_t>& data
    ) {
        can_frame frame;
        std::memset(&frame, 0, sizeof(frame));

        frame.can_id = sdo_tx_cob_id();
        frame.can_dlc = 8;

        // SDO command byte for expedited write
        // Bits 7-5: ccs = 001 (download request)
        // Bit 4: reserved = 0
        // Bit 3: expedited = 1
        // Bit 2: size indicated = 1
        // Bits 1-0: n = (4 - data_size) (number of bytes that do NOT contain data)
        uint8_t n = 4 - data.size();
        frame.data[0] = 0x23 | (n << 2); // 0x23 = expedited write with size

        // Index (little-endian)
        frame.data[1] = index & 0xFF;
        frame.data[2] = (index >> 8) & 0xFF;

        // Subindex
        frame.data[3] = subindex;

        // Data (little-endian, already in correct byte order from to_raw())
        for (size_t i = 0; i < data.size() && i < 4; ++i) {
            frame.data[4 + i] = data[i];
        }

        return frame;
    }

    can_frame SDOClient::create_sdo_read_request(uint16_t index, uint8_t subindex) {
        can_frame frame;
        std::memset(&frame, 0, sizeof(frame));

        frame.can_id = sdo_tx_cob_id();
        frame.can_dlc = 8;

        // SDO command byte: 0x40 = initiate upload request (read)
        frame.data[0] = 0x40;

        // Index (little-endian)
        frame.data[1] = index & 0xFF;
        frame.data[2] = (index >> 8) & 0xFF;

        // Subindex
        frame.data[3] = subindex;

        // Bytes 4-7 are reserved (zeros)

        return frame;
    }

    bool SDOClient::send_frame(const can_frame& frame) {
        std::cout << "[SDO] TX: ID=0x" << std::hex << frame.can_id << " Data=";
        for (int i = 0; i < frame.can_dlc; ++i) {
            std::cout << std::setw(2) << std::setfill('0') << static_cast<int>(frame.data[i]) <<
                " ";
        }
        std::cout << std::dec << std::endl;

        ssize_t bytes_sent = send(socket_fd_, &frame, sizeof(frame), 0);
        return bytes_sent == sizeof(frame);
    }

    bool SDOClient::receive_frame(can_frame& frame, std::chrono::milliseconds timeout) {
        struct pollfd pfd;
        pfd.fd = socket_fd_;
        pfd.events = POLLIN;

        int poll_result = poll(&pfd, 1, timeout.count());

        if (poll_result < 0) {
            std::cerr << "[SDO] Poll error: " << strerror(errno) << "\n";
            return false;
        }

        if (poll_result == 0) {
            // Timeout
            return false;
        }

        ssize_t bytes_received = recv(socket_fd_, &frame, sizeof(frame), 0);
        if (bytes_received != sizeof(frame)) {
            return false;
        }

        std::cout << "[SDO] RX: ID=0x" << std::hex << frame.can_id << " Data=";
        for (int i = 0; i < frame.can_dlc; ++i) {
            std::cout << std::setw(2) << std::setfill('0') << static_cast<int>(frame.data[i]) <<
                " ";
        }
        std::cout << std::dec << std::endl;

        return true;
    }

    bool SDOClient::validate_sdo_response(
        const can_frame& frame,
        uint16_t expected_index,
        uint8_t expected_subindex
    ) {
        // Check COB-ID
        if (frame.can_id != sdo_rx_cob_id()) {
            std::cerr << "[SDO] Wrong COB-ID: expected 0x" << std::hex << sdo_rx_cob_id()
                      << ", got 0x" << frame.can_id << std::dec << "\n";
            return false;
        }

        // Check command byte
        // 0x60 = download response (write confirmation)
        // 0x43, 0x4B, 0x47, 0x4F = upload response (read data, with different sizes)
        // 0x80 = SDO abort
        uint8_t cmd = frame.data[0];

        if (cmd == 0x80) {
            // SDO Abort
            uint32_t abort_code = frame.data[4] | (frame.data[5] << 8) |
                (frame.data[6] << 16) | (frame.data[7] << 24);
            std::cerr << "[SDO] SDO Abort! Code: 0x" << std::hex << abort_code << std::dec << "\n";
            return false;
        }

        if (cmd != 0x60 && (cmd & 0x40) != 0x40) {
            std::cerr << "[SDO] Unexpected command byte: 0x" << std::hex << static_cast<int>(cmd) <<
                std::dec << "\n";
            return false;
        }

        // Verify index/subindex
        uint16_t response_index = frame.data[1] | (frame.data[2] << 8);
        uint8_t response_subindex = frame.data[3];

        if (response_index != expected_index || response_subindex != expected_subindex) {
            std::cerr << "[SDO] Index mismatch: expected 0x" << std::hex << expected_index
                      << "." << static_cast<int>(expected_subindex)
                      << ", got 0x" << response_index << "." << static_cast<int>(response_subindex)
                      << std::dec << "\n";
            return false;
        }

        return true;
    }

} // namespace canopen

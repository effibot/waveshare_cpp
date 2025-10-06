#include "../include/frame/fixed_frame.hpp"
#include "../include/enums/protocol.hpp"
#include <algorithm>

namespace USBCANBridge {
    // === FixedFrame impl_*() Methods ===
    Format FixedFrame::impl_get_format() const {
        std::byte frame_fmt = frame_storage_[layout_.FORMAT];
        return from_byte<Format>(frame_fmt);
    }
    void FixedFrame::impl_set_format(Format format) {
        frame_storage_[layout_.FORMAT] = to_byte(format);
        // Mark checksum as dirty since we changed the frame
        checksum_interface_.mark_dirty();
    }

    uint32_t FixedFrame::impl_get_can_id() const {
        // ID is stored in 4 bytes, little-endian
        return bytes_to_int_le<uint32_t>(get_can_id_span());
    }
    void FixedFrame::impl_set_id(uint32_t id) {
        // ID is stored in 4 bytes, little-endian (fixed size for FixedFrame)
        constexpr std::size_t id_size = layout_t<FixedFrame>::ID_SIZE;
        auto id_bytes = int_to_bytes_le<uint32_t, id_size>(id);
        auto id_span = get_can_id_span();
        // Overwrite the ID bytes in the frame storage
        std::copy(id_bytes.begin(), id_bytes.end(), id_span.begin());
        // Mark checksum as dirty since we changed the frame
        checksum_interface_.mark_dirty();
    }

    void FixedFrame::impl_set_dlc(std::size_t dlc) {
        if (dlc > layout_t<FixedFrame>::DATA_SIZE) {
            throw std::out_of_range("DLC exceeds maximum for FixedFrame");
        }
        frame_storage_[layout_.DLC] = static_cast<std::byte>(dlc);
        // Mark checksum as dirty since we changed the frame
        checksum_interface_.mark_dirty();
    }

    std::size_t FixedFrame::impl_get_dlc() const {
        return static_cast<std::size_t>(frame_storage_[layout_.DLC]);
    }

    span<const std::byte> FixedFrame::impl_get_data() const {
        std::size_t dlc = impl_get_dlc();
        return frame_storage_.subspan(layout_.DATA, dlc);
    }
    void FixedFrame::impl_set_data(span<const std::byte> data) {
        std::size_t dlc = data.size();
        if (dlc > layout_t<FixedFrame>::DATA_SIZE) {
            throw std::out_of_range("Data size exceeds maximum for FixedFrame");
        }
        // Copy data into frame storage
        std::copy(data.begin(), data.end(), frame_storage_.begin() + layout_.DATA);
        // Update DLC accordingly
        impl_set_dlc(dlc);
        // Mark checksum as dirty since we changed the frame
        checksum_interface_.mark_dirty();
    }

    bool FixedFrame::impl_is_extended() const {
        // Retrieve the CANVERS byte
        std::byte canvers = frame_storage_[layout_.CANVERS];
        // Check if it indicates extended ID
        return from_byte<CANVersion>(canvers) == CANVersion::EXT_FIXED;
    }

} // namespace USBCANBridge
#include "../include/frame/variable_frame.hpp"


namespace USBCANBridge {

    // === Core impl_*() Methods ===
    Type VariableFrame::impl_get_type() const {
        return var_type_interface_.get_type();
    }

    void VariableFrame::impl_set_type(CANVersion ver, Format fmt) {
        // Update internal state
        current_version_ = ver;
        current_format_ = fmt;
        std::size_t dlc = impl_get_dlc();
        // Mark Type field as dirty to ensure it's recomputed before sending
        var_type_interface_.mark_dirty();
        // Update the Type field immediately
        var_type_interface_.update_type(current_version_, current_format_, dlc);
        return;
    }

    bool VariableFrame::impl_is_extended() const {
        return current_version_ == CANVersion::EXT_VARIABLE;
    }
    // === Data impl_*() Methods ===
    void VariableFrame::impl_set_dlc(std::size_t dlc) {
        if (dlc > traits_.MAX_DATA_SIZE) {
            throw std::out_of_range("DLC exceeds maximum for VariableFrame");
        }
        // Update the cached DLC
        current_dlc_ = dlc;
        // Mark Type field as dirty to ensure it's recomputed before sending
        var_type_interface_.mark_dirty();
    }

    std::size_t VariableFrame::impl_get_dlc() const {
        return current_dlc_;
    }
    CANVersion VariableFrame::impl_get_CAN_version() const {
        return current_version_;
    }
    void VariableFrame::impl_set_CAN_version(CANVersion ver) {
        // Update internal state if different
        if (current_version_ == ver) {
            return;
        }
        current_version_ = ver;
        // Mark Type field as dirty to ensure it's recomputed before sending
        var_type_interface_.mark_dirty();
        // Update the ID field size if needed
        std::size_t id_size = layout_.id_size(impl_is_extended());
        auto [first_part, second_part] = split_buffer(layout_.ID);
        // Resize the first part to accommodate the new ID size
        first_part.resize(layout_.ID + id_size);
        // Merge back the buffers
        merge_buffer(first_part, second_part);
        return;
    }
    Format VariableFrame::impl_get_format() const {
        return current_format_;
    }
    void VariableFrame::impl_set_format(Format format) {
        current_format_ = format;
        // Mark Type field as dirty to ensure it's recomputed before sending
        var_type_interface_.mark_dirty();
    }
    uint32_t VariableFrame::impl_get_CAN_id() const {
        // ID is stored in 2 or 4 bytes, little-endian
        return bytes_to_int_le<uint32_t>(get_CAN_id_span());
    }
    void VariableFrame::impl_set_CAN_id(uint32_t id) {
        // ID is stored in 2 or 4 bytes, little-endian (variable size for VariableFrame)
        std::size_t id_size = layout_.id_size(impl_is_extended());
        auto id_bytes = int_to_bytes_le<uint32_t>(id);
        auto id_span = get_CAN_id_span();
        // Check that the span is large enough and expand if necessary
        if (id_span.size() < id_size) {
            auto [first_part, second_part] = split_buffer(layout_.ID);
            // Resize the first part to accommodate the larger ID
            first_part.resize(layout_.ID + id_size);
            // Merge back the buffers
            merge_buffer(first_part, second_part);
            // Refresh the id_span after resizing
            id_span = get_CAN_id_span();
        }
        // Overwrite the ID bytes in the frame storage
        std::copy(id_bytes.begin(), id_bytes.begin() + id_size, id_span.begin());
        // Mark Type field as dirty to ensure it's recomputed before sending
        var_type_interface_.mark_dirty();
    }

    span<const std::byte> VariableFrame::impl_get_data() const {
        std::size_t dlc = impl_get_dlc();
        return frame_storage_.subspan(layout_.data(impl_is_extended()), dlc);
    }

    void VariableFrame::impl_set_data(span<const std::byte> data) {
        std::size_t dlc = data.size();
        if (dlc > traits_.MAX_DATA_SIZE) {
            throw std::out_of_range("Data size exceeds maximum for VariableFrame");
        }
        // Mark Type field as dirty to ensure it's recomputed before sending
        var_type_interface_.mark_dirty();
        // If DLC has changed, we may need to resize the internal buffer
        if (dlc != current_dlc_) {
            // Update the cached DLC
            current_dlc_ = dlc;
            auto [first_part, second_part] = split_buffer(layout_.data(impl_is_extended()));
            // Resize the first part to accommodate the new data size
            first_part.resize(layout_.data(impl_is_extended()) + dlc);
            // Merge back the buffers
            merge_buffer(first_part, second_part);
        }
        // Copy data into frame storage
        std::copy(data.begin(), data.end(),
            frame_storage_.begin() + layout_.data(impl_is_extended()));
        return;
    }


} // namespace USBCANBridge
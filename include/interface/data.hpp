#pragma once
#include "core.hpp"
#include <type_traits>
#include <boost/core/span.hpp>

using namespace boost;

namespace USBCANBridge {
    template<typename Frame>
    class DataInterface : public CoreInterface<Frame> {
        // * Alias for traits
        using traits = frame_traits_t<Frame>;
        using layout = layout_t<Frame>;
        using storage = storage_t<Frame>;

        // * Ensure that Derived is VariableFrame or FixedFrame
        static_assert(is_data_frame_v<Frame>,
            "Derived must be a data frame type");

        protected:

            // * Prevent this class from being instantiated directly
            DataInterface() : CoreInterface<Frame>() {
                static_assert(!std::is_same_v<Frame, DataInterface>,
                    "DataInterface cannot be instantiated directly");
            }


        // === Utility Methods ===
        public:
        public:
            /**
             * @brief Find if the frame is a remote frame.
             * Remote frames are determined by the FORMAT byte:
             * - For Fixed frames: REMOTE_FIXED (0x02)
             * - For Variable frames: REMOTE_VARIABLE (0x01)
             * Since DATA_FIXED and REMOTE_VARIABLE both use 0x01, we need to check
             * the frame type (via CAN_VERS) to distinguish them.
             * @return bool True if remote frame, false otherwise
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, bool>
            is_remote() const {
                auto fmt = this->derived().impl_get_format();
                auto vers = this->derived().impl_get_CAN_version();

                // Check if this is a fixed or variable frame
                bool is_fixed = (vers == CANVersion::STD_FIXED || vers == CANVersion::EXT_FIXED);
                bool is_variable = (vers == CANVersion::STD_VARIABLE ||
                    vers == CANVersion::EXT_VARIABLE);

                if (is_fixed) {
                    return fmt == Format::REMOTE_FIXED;  // 0x02
                } else if (is_variable) {
                    return fmt == Format::REMOTE_VARIABLE;  // 0x01
                }

                return false;  // Unknown frame type
            }

            /**
             * @brief Check if the frame is extended (29-bit ID) or standard (11-bit ID).
             * @return bool True if extended, false if standard
               ```
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, bool>
            is_extended() const {
                return this->derived().impl_is_extended();
            }

            /**
             * @brief Get the size of the id field
             * This methods use the frame layout and the current is_extended() value to determine the size of the id field.
             * @return std::size_t The size of the id field in bytes
             */
            std::size_t get_id_field_size() const {
                if constexpr (is_fixed_frame_v<Frame>) {
                    return layout::ID_SIZE;
                }
                if constexpr (is_variable_frame_v<Frame>) {
                    return this->layout_.id_size(is_extended());
                }

            }

            /**
             * @brief Get a subspan to access the CAN ID field in the internal storage.
             * @return span<std::uint8_t> A subspan representing the ID field
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, span<std::uint8_t> >
            get_CAN_id_span() {
                return this->get_storage().subspan(
                    this->layout_.ID,
                    this->get_id_field_size()
                );
            }

            /**
             * @brief Get a subspan to access the CAN ID field in the internal storage.
             * @return span<std::uint8_t> A subspan representing the ID field
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, span<const std::uint8_t> >
            get_CAN_id_span() const {
                const storage_t<T>& frame_storage = this->get_storage();
                const size_t id_size = this->get_id_field_size();
                return frame_storage.subspan(
                    this->layout_.ID,
                    id_size
                );
            }


        // === Data Frame Specific Methods ===

        private:
            /**
             * @brief Set the dlc object
             * This is designed to be called only by the set_data() method, so that the dlc is always consistent with the actual data size.
             * This calls derived().set_dlc() for frame-specific dlc setting.
             * @tparam T
             * @param dlc
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, void>
            set_dlc(std::size_t dlc) {
                this->derived().impl_set_dlc(dlc);
            }

        public:

            /**
             * @brief Get the Frame Format object
             * @return Format The frame format
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, Format>
            get_format() const {
                return this->derived().impl_get_format();
            }
            /**
             * @brief Set the Frame Format object
             * @param format The format to set
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, void>
            set_format(Format format) {
                this->derived().impl_set_format(format);
            }

            /**
             * @brief Get the id object
             * Get the CAN ID of the data frame.
             * This calls derived().get_can_id() for frame-specific id retrieval.
             * @tparam T
             * @return uint32_t The CAN ID
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, std::uint32_t>
            get_can_id() const {
                return this->derived().impl_get_CAN_id();
            }
            /**
             * @brief Set the id object
             * Set the CAN ID of the data frame.
             * This calls derived().set_id() for frame-specific id setting.
             * @tparam T
             * @param id The CAN ID to set
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, void>
            set_id(std::uint32_t id) {
                this->derived().impl_set_CAN_id(id);
            }

            /**
             * @brief Get the dlc object
             * Get the Data Length Code (DLC) of the data frame.
             * This calls derived().get_dlc() for frame-specific dlc retrieval.
             * @tparam T
             * @return std::size_t The DLC value
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, std::size_t>
            get_dlc() const {
                return this->derived().impl_get_dlc();
            }

            /**
             * @brief Get a modifiable view of the data payload.
             * @return span<std::uint8_t> Mutable view of the data
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, span<std::uint8_t> >
            get_data() {
                return this->derived().impl_get_data();
            }

            /**
             * @brief Get a read-only view of the data payload.
             * @return span<const std::uint8_t> View of the data
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, span<const std::uint8_t> >
            get_data() const {
                return this->derived().impl_get_data();
            }

            /**
             * @brief Set the data object
             * Set the data payload of the data frame and update the dlc accordingly.
             * This calls derived().impl_set_data() for frame-specific data setting, and also updates the dlc accordingly.
             * @tparam T
             * @param data A span representing the data payload to set.
             */
            template<typename T = Frame>
            std::enable_if_t<is_data_frame_v<T>, void>
            set_data(span<const std::uint8_t> data) {
                this->derived().impl_set_data(data);
                set_dlc(data.size());
            }
    };

}
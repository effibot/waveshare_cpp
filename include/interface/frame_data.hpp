#pragma once
#include "frame_core.hpp"
#include <type_traits>

namespace USBCANBridge {
    template<typename Derived>
    class Data : public Core<Derived> {
        // * Ensure that Derived is VariableFrame or FixedFrame
        static_assert(is_data_frame_v<Derived>,
            "Derived must be a data frame type");


        // === Utility Methods ===
        public:
            /**
             * @brief Find if the frame is a remote frame.
             * @return std::enable_if_t<is_data_frame_v<T>, Result<bool> >
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<bool> >
            is_remote() const {
                auto fmt_res = this->derived().impl_get_format();
                if (!fmt_res) {
                    return fmt_res.error();
                }
                return fmt_res.value() == FrameFmt::REMOTE_FIXED ||
                       fmt_res.value() == FrameFmt::REMOTE_VARIABLE;
            }

        // === Data Frame Specific Methods ===

        private:
            /**
             * @brief Set the dlc object
             * This is designed to be called only by the set_data() method, so that the dlc is always consistent with the actual data size.
             * This calls derived().set_dlc() for frame-specific dlc setting.
             * @tparam T
             * @param dlc
             * @return std::enable_if_t<is_data_frame_v<T>, Result<void> >
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<void> >
            set_dlc(std::size_t dlc) {
                return this->derived().impl_set_dlc(dlc);
            }
        public:

            // * Default constructor and destructor
            Data() = default;
            ~Data() = default;

            /**
             * @brief Get the Frame Format object
             * @return std::enable_if_t<is_data_frame_v<T>, Result<FrameFmt> >
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<FrameFmt> >
            get_format() const {
                return this->derived().impl_get_format();
            }
            /**
             * @brief Set the Frame Format object
             * @param format
             * @return std::enable_if_t<is_data_frame_v<T>, Result<void> >
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<void> >
            set_format(FrameFmt format) {
                return this->derived().impl_set_format(format);
            }

            /**
             * @brief Get the id object
             * Get the CAN ID of the data frame.
             * This calls derived().get_id() for frame-specific id retrieval.
             * @tparam T
             * @return std::enable_if_t<is_data_frame_v<T>, Result<std::uint32_t> >
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<std::uint32_t> >
            get_id() const {
                return this->derived().impl_get_id();
            }
            /**
             * @brief Set the id object
             * Set the CAN ID of the data frame.
             * This calls derived().set_id() for frame-specific id setting.
             * @tparam T
             * @param id
             * @return std::enable_if_t<is_data_frame_v<T>, Result<void> >
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<void> >
            set_id(std::uint32_t id) {
                return this->derived().impl_set_id(id);
            }
            /**
             * @brief Get the dlc object
             * Get the Data Length Code (DLC) of the data frame.
             * This calls derived().get_dlc() for frame-specific dlc retrieval.
             * @tparam T
             * @return std::enable_if_t<is_data_frame_v<T>, Result<std::size_t> >
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<std::size_t> >
            get_dlc() const {
                return this->derived().impl_get_dlc();
            }
            /**
             * @brief Set the data object
             * Set the data payload of the data frame and update the dlc accordingly.
             * This calls derived().set_data() for frame-specific data setting, and also updates the dlc accordingly.
             * @tparam T
             * @param data A span representing the data payload to set.
             * @return std::enable_if_t<is_data_frame_v<T>, Result<void> >
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<void> >
            set_data(span<const std::byte> data) {
                auto res = this->derived().impl_set_data(data);
                if (!res) {
                    return res;
                }
                return set_dlc(data.size());
            }

            /**
             * @brief Get a read-only view of the data payload.
             * @return std::enable_if_t<is_data_frame_v<T>, Result<span<const std::byte>>>
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<span<const std::byte> > >
            get_data() const {
                return this->derived().impl_get_data();
            }

            /**
             * @brief Get a modifiable view of the data payload.
             * @return std::enable_if_t<is_data_frame_v<T>, Result<span<std::byte>>>
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<span<std::byte> > >
            get_data() {
                return this->derived().impl_get_data();
            }
    };

}
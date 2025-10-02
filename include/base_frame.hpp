#pragma once

#include "common.hpp"
#include "result.hpp"
#include "frame_traits.hpp"
#include "span_compat.hpp"
#include <type_traits>

namespace USBCANBridge {

/**
 * @brief CRTP base class providing unified interface with type-safe operations.
 *
 * This class serves as the main entry point for the library while using SFINAE
 * to ensure only appropriate operations are available for each frame type.
 * All operations are dispatched to derived implementations at compile-time.
 */
    template<typename Derived>
    class BaseFrame {
        protected:
            // * Bring traits into scope
            using Traits = frame_traits_t<Derived>;
            using StorageType = storage_t<Derived>;

        private:
            // * Access to the concrete Derived class
            Derived& derived() {
                return static_cast<Derived&>(*this);
            }
            const Derived& derived() const {
                return static_cast<const Derived&>(*this);
            }



        public:
            // * Default constructor
            BaseFrame() = default;
            ~BaseFrame() = default;

        // === UNIVERSAL OPERATIONS - Available for ALL frame types ===
        private:
            // * Initialize fixed fields
            void init_fixed_fields() {
                if constexpr (std::is_same_v<Derived, FixedFrame> ) {
                    derived().impl_init_fixed_fields();
                }
            }

        public:
            /**
             * @brief Serialize frame to byte buffer.
             */
            Result<void> serialize(span<std::byte> buffer) const {
                return derived().impl_serialize(buffer);
            }

            /**
             * @brief Deserialize frame from byte buffer.
             */
            Result<void> deserialize(span<const std::byte> data) {
                return derived().impl_deserialize(data);
            }

            /**
             * @brief Validate frame structure and content.
             */
            Result<bool> validate() const {
                return derived().impl_validate();
            }

            /**
             * @brief Get raw frame data as read-only byte span.
             */
            span<const std::byte> get_raw_data() const {
                return derived().impl_get_raw_data();
            }

            /**
             * @brief Get frame size in bytes.
             */
            Result<std::size_t> size() const {
                return derived().impl_size();
            }

            /**
             * @brief Clear frame to default state.
             */
            Result<void> clear() {
                return derived().impl_clear();
            }

            // === DATA FRAME OPERATIONS - Only available for FixedFrame and VariableFrame ===

            /**
             * @brief Set CAN ID (only available for data frames).
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<void> >
            set_can_id(uint32_t id) {
                return derived().impl_set_can_id(id);
            }

            /**
             * @brief Get CAN ID (only available for data frames).
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<uint32_t> >
            get_can_id() const {
                return derived().impl_get_can_id();
            }

            /**
             * @brief Set data payload (only available for data frames).
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<void> >
            set_data(span<const std::byte> data) {
                return derived().impl_set_data(data);
            }

            /**
             * @brief Get data payload (only available for data frames).
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, span<const std::byte> >
            get_data() const {
                return derived().impl_get_data();
            }

            /**
             * @brief Set DLC (Data Length Code) (only available for data frames).
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<void> >
            set_dlc(std::byte dlc) {
                return derived().impl_set_dlc(dlc);
            }

            /**
             * @brief Get DLC (Data Length Code) (only available for data frames).
             */
            template<typename T = Derived>
            std::enable_if_t<is_data_frame_v<T>, Result<std::byte> >
            get_dlc() const {
                return derived().impl_get_dlc();
            }

            // === CONFIG FRAME OPERATIONS - Only available for ConfigFrame ===

            /**
             * @brief Set baud rate (only available for config frames).
             */
            template<typename T = Derived>
            std::enable_if_t<is_config_frame_v<T>, Result<void> >
            set_baud_rate(CANBaud rate) {
                return derived().impl_set_baud_rate(rate);
            }

            /**
             * @brief Get baud rate (only available for config frames).
             */
            template<typename T = Derived>
            std::enable_if_t<is_config_frame_v<T>, Result<CANBaud> >
            get_baud_rate() const {
                return derived().impl_get_baud_rate();
            }

            /**
             * @brief Set ID filter (only available for config frames).
             */
            template<typename T = Derived>
            std::enable_if_t<is_config_frame_v<T>, Result<void> >
            set_filter(uint32_t filter) {
                return derived().impl_set_filter(filter);
            }
            /**
             * @brief Get ID filter (only available for config frames).
             */
            template<typename T = Derived>
            std::enable_if_t<is_config_frame_v<T>, Result<uint32_t> >
            get_filter() const {
                return derived().impl_get_filter();
            }
            /**
             * @brief Set ID mask (only available for config frames).
             */
            template<typename T = Derived>
            std::enable_if_t<is_config_frame_v<T>, Result<void> >
            set_mask(uint32_t mask) {
                return derived().impl_set_mask(mask);
            }

            /**
             * @brief Get ID mask (only available for config frames).
             */
            template<typename T = Derived>
            std::enable_if_t<is_config_frame_v<T>, Result<uint32_t> >
            get_mask() const {
                return derived().impl_get_mask();
            }

            /**
             * @brief Set operating mode (only available for config frames).
             */
            template<typename T = Derived>
            std::enable_if_t<is_config_frame_v<T>, Result<void> >
            set_mode(CANMode mode) {
                return derived().impl_set_mode(mode);
            }
            /**
             * @brief Get operating mode (only available for config frames).
             */
            template<typename T = Derived>
            std::enable_if_t<is_config_frame_v<T>, Result<CANMode> >
            get_mode() const {
                return derived().impl_get_mode();
            }
            /**
             * @brief Set the Auto RTX byte
             * @param auto_rtx New Auto RTX value
             */
            template<typename T = Derived>
            std::enable_if_t<is_config_frame_v<T>, Result<void> >
            set_auto_rtx(RTX auto_rtx) {
                return derived().impl_set_auto_rtx(auto_rtx);
            }
            /**
             * @brief Get the Auto RTX byte
             * @return Current Auto RTX value
             */
            template<typename T = Derived>
            std::enable_if_t<is_config_frame_v<T>, Result<RTX> >
            get_auto_rtx() const {
                return derived().impl_get_auto_rtx();
            }

            // === FIXED FRAME SPECIFIC OPERATIONS ===

            /**
             * @brief Set frame type (only available for FixedFrame).
             */
            template<typename T = Derived>
            std::enable_if_t<std::is_same_v<T, FixedFrame>, Result<void> >
            set_frame_type(FrameType type) {
                return derived().impl_set_frame_type(type);
            }


        // === CHECKSUM INTERFACE (for Fixed and Config frames) ===
        private:
            /**
             * @brief Verify checksum (only available for FixedFrame and ConfigFrame).
             */
            template<typename T = Derived>
            std::enable_if_t<has_checksum_v<T>, Result<bool> >
            verify_checksum() const {
                return derived().impl_verify_checksum();
            }
            /**
             * @brief Recalculate and update checksum (only available for FixedFrame and ConfigFrame).
             */
            template<typename T = Derived>
            std::enable_if_t<has_checksum_v<T>, void>
            update_checksum() {
                return derived().update_checksum();
            }
            /**
             * @brief Reset checksum state (only available for FixedFrame and ConfigFrame).
             */
            template<typename T = Derived>
            std::enable_if_t<has_checksum_v<T>, void>
            mark_checksum_dirty() {
                return derived().dirty_ = true;
            }
            /**
             * @brief Calculate the checksum from the internal storage (only available for FixedFrame and ConfigFrame).
             */
            template<typename T = Derived>
            std::enable_if_t<has_checksum_v<T>, std::byte>
            calculate_checksum() const {
                return derived().calculate_checksum();
            }


    };

} //  namespace USBCANBridge

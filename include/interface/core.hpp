/**
 * @file core.hpp
 * @author Andrea Efficace (andrea.efficace@)
 * @brief
 * @version 0.1
 * @date 2025-10-02
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include <boost/core/span.hpp>
#include <cstddef>
#include <type_traits>

#include "../enums/protocol.hpp"
#include "../template/result.hpp"
#include "../template/frame_traits.hpp"

#include "checksum.hpp"

using namespace boost;

namespace USBCANBridge {
    /**
     * @brief Core interface for all frame types
     * This class provides common methods and utilities for all frame types.
     * @tparam Frame The frame type to interface with.
     */
    template<typename Frame>
    class CoreInterface {

        protected:
            // * Reference to traits
            using frame_traits_t = FrameTraits<Frame>;
            frame_traits_t traits_;
            // * Frame layout
            alignas(alignof(layout_t<Frame>)) mutable layout_t<Frame> layout_;
            // * Frame storage
            alignas(alignof(storage_t<Frame>)) mutable storage_t<Frame> frame_storage_;

            // * CRTP helper to access derived class methods
            Frame& derived() {
                return static_cast<Frame&>(*this);
            }

            // * CRTP helper to access derived class methods (const version)
            const Frame& derived() const {
                return static_cast<const Frame&>(*this);
            }

            // * Prevent this class from being instantiated directly
            CoreInterface() : frame_storage_(traits_.frame_buffer) {
                static_assert(!std::is_same_v<Frame, CoreInterface>,
                    "CoreInterface cannot be instantiated directly");
                // Initialize constant fields
                init_fields();
            }

        private:

            /**
             * @brief Initialize constants fields in the frame storage.
             * This method is called by the derived class constructor (or by the `clear()`) method to set (or reset) the initial values of the constant fields.
             */
            void init_fields() {
                // Initialize the frame storage with default values
                std::fill(std::begin(frame_storage_), std::end(frame_storage_), std::byte(0x00));
                // Set the Start byte
                frame_storage_[layout_.START] = to_byte(Constants::START_BYTE);
                // Call derived class to initialize fixed fields
                derived().impl_init_fields();
                return;
            }

        public:

            // === Access to Internal Storage ===
            /**
             * @brief Get a mutable reference to the internal frame storage.
             * This allows read-write access to the raw frame data.
             * @return storage_t& Mutable reference to the internal frame storage.
             */
            storage_t<Frame>& get_storage() {
                return frame_storage_;
            }
            /**
             * @brief Get a const reference to the internal frame storage.
             * This allows read-only access to the raw frame data.
             * @return const storage_t& Const reference to the internal frame storage.
             */
            const storage_t<Frame>& get_storage() const {
                return frame_storage_;
            }


            // === Access to Layout ===
            /**
             * @brief Get a const reference to the frame layout.
             * This allows read-only access to the layout information.
             * @return const layout_t& Const reference to the frame layout.
             */
            const layout_t<Frame>& get_layout() const {
                return layout_;
            }

            // === Common Frame Methods ===

            /**
             * @brief Clear the frame data.
             * Reset the frame to its initial state.
             * @note This will zero out the frame storage and re-initialize constant fields.
             */
            void clear() {
                std::fill(std::begin(frame_storage_), std::end(frame_storage_), 0x00);
                init_fields();
            }

            /**
             * @brief Print the frame in a human-readable format.
             * @return std::string A string representation of the frame.
             * @note This calls derived().to_string() for frame-specific string conversion.
             */
            std::string to_string() const {
                return dump_frame<Frame>(frame_storage_);
            }
            /**
             * @brief Get the size of the frame in bytes.
             * @note If the frame has a fixed size, this returns that size. If variable, it calls derived() to compute the size.
             * @return std::size_t The size of the frame in bytes.
             */
            template<typename T = Frame>
            std::size_t size() const {
                if  constexpr (!is_variable_frame_v<T>) {
                    return frame_traits_t::FRAME_SIZE;
                }
                return derived().impl_size();

            }

            // === Protocol-Specific Methods ===
            /**
             * @brief Get the Type byte of the frame.
             * @return Type The Type byte of the frame.
             * @note This calls derived().get_type() for frame-specific type retrieval.
             */
            Type get_type() const {
                return derived().impl_get_type();
            }

            /**
             * @brief Set the Type byte of the frame.
             * @param type The Type byte to set.
             * @note This calls derived().set_type() for frame-specific type setting.
             */
            template<typename T = Frame>
            std::enable_if_t<!is_variable_frame_v<T>, void>
            set_type(Type type) {
                derived().impl_set_type(type);
            }

            /**
             * @brief Set the Type byte of the frame.
             * @param ver The CANVersion byte to set.
             * @param fmt The Format byte to set.
             * @note This calls derived().get_type() for frame-specific type retrieval.
             */
            template<typename T = Frame>
            std::enable_if_t<is_variable_frame_v<T>, void>
            set_type(CANVersion ver, Format fmt) {
                derived().impl_set_type(ver, fmt);
            }

            /**
             * @brief Get the CANVersion byte of the frame.
             * This tells wether the frame uses standard or extended CAN IDs.
             * @return CANVersion The CANVersion byte of the frame.
             * @note This calls derived().get_can_version() for frame-specific frame type retrieval.
             */
            CANVersion get_CAN_vesion() const {
                return derived().impl_get_CAN_version();
            }

            /**
             * @brief Set the CANVersion byte of the frame.
             * @param frame_type The CANVersion byte to set.
             * @note This calls derived().set_CAN_version() for frame-specific frame type setting.
             */
            template<typename T = Frame>
            std::enable_if_t<is_variable_frame_v<T>, void>
            set_CAN_version(CANVersion frame_type) {
                derived().impl_set_CAN_version(frame_type);
            }
    };
}


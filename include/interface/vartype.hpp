#pragma once
#include "../enums/protocol.hpp"
#include "../template/frame_traits.hpp"

namespace USBCANBridge {

    /**
     * @brief Utility class to handle the reconstruction of the Type field
     * in a Variable size Frame.
     * The Type field is composed of:
     *
     * - Bits 7-6: constant Type byte (`Constants::DATA_VARIABLE`)
     *
     * - Bit 5: CAN version (`CANVersion::STD_VARIABLE` or `CANVersion::EXT_VARIABLE`)
     *
     * - Bit 4: Frame format (`Format::DATA_VARIABLE` or `Format::REMOTE_VARIABLE`)
     *
     * - Bits 3-0: Data Length Code (DLC), number of data bytes (0-8)
     *
     * @see File: `include/enums/protocol.hpp` for definitions.
     * @tparam Frame
     */
    template<typename Frame>
    class VarTypeInterface {
        private:
            // * Reference to the associated frame
            Frame& frame_;

            Frame& get_frame() {
                return frame_;
            }

            const Frame& get_frame() const {
                return frame_;
            }

            // * CRTP helper to access layout
            constexpr static std::size_t TYPE = layout_t<Frame>::TYPE;

            // # Internal State
            bool is_dirty_ = true; // To track if frame has been modified
            std::byte& type_field_;


        public:
            // * Constructor
            VarTypeInterface(Frame& frame) : frame_(frame),
                type_field_(frame.get_storage()[TYPE]) {
                static_assert(is_variable_frame_v<Frame>,
                    "VarTypeInterface can only be used with VariableFrame");
            }

        // === dirty bit Operations ===

        private:
            /**
             * @brief Mark the Type field as clean (i.e., up-to-date).
             * This should be called whenever the Type field is successfully updated.
             */
            void mark_clean() {
                is_dirty_ = false;
            }

        public:
            /**
             * @brief Mark the Type field as dirty (i.e., needing recomputation).
             * This should be called whenever the frame data changes.
             */
            void mark_dirty() {
                is_dirty_ = true;
            }

            /**
             * @brief Check if the Type field is dirty (i.e., needs to be recomputed).
             * @return True if the Type field is dirty, false otherwise.
             *
             */
            bool is_dirty() const {
                return is_dirty_;
            }

            // === Protocol Static Utiliy Functions ===
            /**
             * @brief Extract the Type base byte from a Type byte.
             * The Type base is a couple of bits that identify the frame as
             * a variable-length frame for the Waveshare USB-CAN adapter protocol.
             * @note The extraction is performed by masking out all but the two highest bits.
             * @param type The Type byte to extract from.
             * @return The extracted Type base byte.
             */
            constexpr static Type extract_type_base(std::byte type) {
                return static_cast<Type>(
                    static_cast<std::uint8_t>(type) & 0xC0);
            }
            /**
             * @brief Extract the CANVersion from a Type byte.
             * The CANVersion is a single bit that indicates whether the frame is
             * standard (STD) or extended (EXT).
             * @note The extraction is performed by right-shifting to select the fifth bit
             * and masking out all other bits.
             * @param type The Type byte to extract from.
             * @return The extracted CANVersion.
             */
            constexpr static CANVersion extract_frame_type(std::byte type) {
                return static_cast<CANVersion>(
                    (static_cast<std::uint8_t>(type) >> 5) & 1);
            }
            /**
             * @brief Extract the Format from a Type byte.
             * The Format is a single bit that indicates whether the frame is
             * a data frame or a remote frame.
             * @note The extraction is performed by right-shifting to select the fourth bit
             * and masking out all other bits.
             * @param type The Type byte to extract from.
             * @return The extracted Format.
             */
            constexpr static Format extract_frame_format(std::byte type) {
                return static_cast<Format>(
                    (static_cast<std::uint8_t>(type) >> 4) & 1);
            }
            /**
             * @brief Extract the DLC from a Type byte.
             * The DLC (Data Length Code) is represented by the lowest four bits of the Type byte,
             * which indicates the number of data bytes in the frame.
             * @note The extraction is performed by masking out all but the four lowest bits using a
             * bitwise AND operation with `0x0F` (`0b1111`).
             * @param type The Type byte to extract from.
             * @return The extracted DLC (0-8).
             */
            constexpr static std::size_t extract_dlc(std::byte type) {
                return static_cast<std::size_t>(static_cast<std::uint8_t>(type) & 0x0F);
            }
            /**
             * @brief Calculate the Type byte for a VariableFrame based on its properties.
             * This function determines the appropriate Type byte for a VariableFrame
             * based on the frame's CANVersion, Format, and data length code (DLC).
             * @param can_ver The CANVersion of the VariableFrame
             * @param frame_fmt The Format of the VariableFrame
             * @param dlc The data length code (DLC) of the VariableFrame
             * @note The Type byte is constructed as follows:
             *
             * - Bits 7-6: Type base byte (0xC0 for VariableFrame)
             *
             * - Bit 5: CANVersion (0 for STD, 1 for EXT)
             *
             * - Bit 4: Format (0 for DATA, 1 for REMOTE)
             *
             * - Bits 3-0: DLC (0-8)
             *
             * @see extract_type_base, extract_frame_type, extract_frame_format,
               extract_dlc to know the reason behind each bit manipulation.
             * @warning We assume that the caller has already validated each parameter.
             * @return The calculated Type byte for the VariableFrame
             */

            constexpr static std::byte compute_type(CANVersion can_ver, Format frame_fmt,
                std::size_t dlc) {
                // Combine all bit operations into a single expression
                return static_cast<std::byte>(
                    static_cast<std::uint8_t>(Type::DATA_VARIABLE) |
                    (static_cast<std::uint8_t>(can_ver) << 5) |
                    (static_cast<std::uint8_t>(frame_fmt) << 4) |
                    (static_cast<std::uint8_t>(dlc) & 0x0F)
                );
            }

        // === Wrapper for Frame usage ===
        public:

            /**
             * @brief Update the Type field if it is marked as dirty.
             * This method checks if the Type field is dirty and, if so,
             * recomputes its value based on the current frame properties.
             * After updating, the Type field is marked as clean.
             * @param can_ver The CANVersion of the VariableFrame
             * @param frame_fmt The Format of the VariableFrame
             * @param dlc The data length code (DLC) of the VariableFrame
             */

            void update_type(CANVersion can_ver, Format frame_fmt, std::size_t dlc) {
                if (is_dirty_) {
                    type_field_ = compute_type(can_ver, frame_fmt, dlc);
                    mark_clean();
                }
            }

            /**
             * @brief Get the Type byte of the VariableFrame.
             * This method returns the computed Type byte of the VariableFrame, but it does not perform any updates.
             * @return The Type byte of the VariableFrame
             * @warning This method should be use just to perform checks and read operations. In fact, this does not update the Type field, even if it is dirty.
             */
            Type get_type() const {
                // compute a new Type byte from the current frame properties
                std::byte new_type = compute_type(
                    get_frame().impl_get_CAN_version(),
                    get_frame().impl_get_format(),
                    get_frame().impl_get_dlc()
                );
                return from_byte<Type>(new_type);
            }


    };
}
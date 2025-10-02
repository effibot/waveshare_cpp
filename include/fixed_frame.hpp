#pragma once

#include "interface/data.hpp"
#include "interface/checksum.hpp"

namespace USBCANBridge {

    class FixedFrame :
        public DataInterface<FixedFrame>,
        public ChecksumInterface<FixedFrame> {
        // * Alias for traits
        using traits = frame_traits_t<FixedFrame>;
        using layout = layout_t<FixedFrame>;
        using storage = storage_t<FixedFrame>;

        protected:
            /**
             * @brief Initialize the frame fields.
             * This is called during construction to set up the frame.
             * @see File: include/protocol.hpp for constant definitions.
             * @see File: README.md for frame structure details.
             * @note For a FixedFrame, the following fields are initialized:
             * - `[START]` = `Constants::START_BYTE`
             * - `[HEADER]` = `Constants::HEADER`
             * - `[TYPE]` = Type::DATA_FIXED
             * - `[FRAME_TYPE]` = `FrameType::STD_FIXED`
             * - `[FRAME_FMT]` = `FrameFmt::DATA_FIXED`
             * - `[RESERVED]` = `Constants::RESERVED`
             */
            void impl_init_fields() {
                // * Set the Start byte
                frame_storage_[layout::START_OFFSET] = to_byte(Constants::START_BYTE);
                // * Set the Header byte
                frame_storage_[layout::HEADER_OFFSET] = to_byte(Constants::HEADER);
                // * Set the Type byte
                frame_storage_[layout::TYPE_OFFSET] = to_byte(Type::DATA_FIXED);
                // * Set the Frame Type byte
                frame_storage_[layout::FRAME_TYPE_OFFSET] = to_byte(FrameType::STD_FIXED);
                // * Set the Frame Format byte
                frame_storage_[layout::FORMAT_OFFSET] = to_byte(FrameFormat::DATA_FIXED);
                // * Set the Reserved byte
                frame_storage_[layout::RESERVED_OFFSET] = to_byte(Constants::RESERVED);
                return;
            }
        public:
            // * Constructors
            FixedFrame() : DataInterface<FixedFrame>(), ChecksumInterface<FixedFrame>() {
                // Initialize constant fields
                impl_init_fields();
            }

    };
}
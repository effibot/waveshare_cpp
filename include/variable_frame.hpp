#pragma once
#include "base_frame.hpp"

namespace USBCANBridge {
    /**
     * @brief Variable frame implementation.
     *
     * This class represents a variable frame in the USBCANBridge library.
     * It inherits from the BaseFrame class and provides specific functionality
     * for variable frames, such as Type calculation and validation, and synchronization
     * of DLC and data payload size.
     * The structure is the following:
     *
     * `[START][TYPE][ID0-ID3][DATA0-DATA7][END]`
     *
     * @note `compute_type()` in `common.hpp` for details on Type field calculation.
     * @note BaseFrame for common frame functionalities.
     * @note FrameTraits for compile-time frame structure information.
     * @note VariableFrame doesn't use a checksum field, but has a terminating END byte.
     */
    class VariableFrame : public BaseFrame<VariableFrame> {
        protected:
            using PayloadPair = Traits::PayloadPair;
            using IDPair = Traits::IDPair;
            alignas(8) mutable StorageType storage_;
            // cached id size value
            mutable size_t id_size_ = 0;
        private:
            /**
             * @brief Initialize fixed fields of the variable frame.
             * @see VariableSizeIndex in common.hpp for field details.
             */
            void init_fixed_fields();

        public:
            // <<< Constructors
            /**
             * @brief Default constructor.
             *
             * Initializes a VariableFrame object with default values.
             */
            VariableFrame() : BaseFrame() {
                init_fixed_fields();
            }
            ~VariableFrame() = default;
            // >>> Constructors

            // ? Base Frame Protocol Interface

            Result<std::size_t> impl_size() const;
            Result<Status> impl_clear();
            Result<Type> impl_get_type() const;
            Result<Status> impl_set_type(const Type& type) = delete; // Disabled for VariableFrame
            Result<Status> impl_set_type(const FrameFmt frame_fmt,
                const FrameType frame_type, const std::byte dlc);
            Result<FrameType> impl_get_frame_type() const;
            Result<Status> impl_set_frame_type(const FrameType& frame_type);
            Result<FrameFmt> impl_get_frame_fmt() const;
            Result<Status> impl_set_frame_fmt(const FrameFmt& frame_fmt);

            // * Wire protocol serialization/deserialization
            Result<StorageType> impl_serialize() const;
            Result<Status> impl_deserialize(const std::byte* data, std::size_t size);

            // * ID accessors
            Result<IDPair> impl_get_id() const;
            Result<Status> impl_set_id(const IDPair& id);

            // * DLC accessors
            Result<std::byte> impl_get_dlc() const;
            Result<Status> impl_set_dlc(const std::byte& dlc);
            Result<PayloadPair> impl_get_data() const;
            Result<Status> impl_set_data(const PayloadPair& new_data);
            // * Validation methods
            Result<Status> impl_validate() const;
            // ? Individual field validation methods
            Result<Status> validate_start_byte() const;
            Result<Status> validate_frame_type(const FrameType& frame_type) const;
            Result<Status> validate_frame_fmt(const FrameFmt& frame_fmt) const;
            Result<Status> validate_id_size(const IDPair& id) const;
            Result<Status> validate_dlc(const std::byte& dlc) const;
            Result<Status> validate_dlc(const std::size_t& dlc) const;
            Result<Status> validate_end_byte() const;

    }; // class VariableFrame
} // namespace USBCANBridge
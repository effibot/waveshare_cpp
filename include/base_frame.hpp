#pragma once
#include <numeric>
#include <algorithm>

#include "common.hpp"
#include "error.hpp"
#include "result.hpp"
#include "frame_traits.hpp"

namespace USBCANBridge {
    template<typename Derived>
    class BaseFrame {
        protected:
            // bring traits into scope
            using Traits = frame_traits_t<Derived>;
            using StorageType = typename Traits::StorageType;
        private:
            // Access to the concrete Derived class
            Derived& derived() {
                return static_cast<Derived&>(*this);
            }
            const Derived& derived() const {
                return static_cast<const Derived&>(*this);
            }

        public:
            // default constructor
            BaseFrame() {
            };
            ~BaseFrame() = default;

            // Disable copy/assignment to avoid slicing
            BaseFrame(const BaseFrame&) = delete;
            BaseFrame& operator=(const BaseFrame&) = delete;
            // Enable move semantics
            BaseFrame(BaseFrame&&) = default;
            BaseFrame& operator=(BaseFrame&&) = default;


            // * Public interface methods calling the implementation in Derived class * //
            /**
             * @brief Get the size of the frame.
             *
             * @return Result<std::size_t>
             */
            Result<std::size_t> size() const {
                return derived().impl_size();
            }
            /**
             * @brief Clear the frame to default state.
             *
             * @return Result<Status>
             */
            Result<Status> clear() {
                return derived().impl_clear();
            }
            /**
             * @brief Retrieves the type of the frame
             * @see common.hpp for Type enum details
             *
             * @return Result<Type>
             */
            Result<Type> get_type() const {
                return derived().impl_get_type();
            }
            /**
             * @brief Set the packet type byte.
             * @see common.hpp for Type enum details
             * @param type
             * @return Result<Status>
             */
            Result<Status> set_type(const Type& type) {
                return derived().impl_set_type(type);
            }
            /**
             * @brief Get the frame type (e.g., standard or extended).
             * @see common.hpp for FrameType enum details
             * @return Result<FrameType>
             */
            Result<FrameType> get_frame_type() const {
                return derived().impl_get_frame_type();
            }
            /**
             * @brief Set the frame type of the packet.
             * @see common.hpp for FrameType enum details
             * @param frame_type
             * @return Result<Status>
             */
            Result<Status> set_frame_type(const FrameType& frame_type) {
                return derived().impl_set_frame_type(frame_type);
            }
            /**
             * @brief Get the frame format of the packet.
             * @see common.hpp for FrameFmt enum details
             * @return Result<FrameFmt>
             */
            Result<FrameFmt> get_frame_fmt() const {
                return derived().impl_get_frame_fmt();
            }
            /**
             * @brief Set the frame format of the packet.
             * @see common.hpp for FrameFmt enum details
             * @param frame_fmt
             * @return Result<Status>
             */
            Result<Status> set_frame_fmt(const FrameFmt& frame_fmt) {
                return derived().impl_set_frame_fmt(frame_fmt);
            }

            // >>> Fields setters/getters

            /**
             * @brief Prepares the frame for wire protocol serialization.
             * The implementation in the derived class must ensure that the frame is valid and return a copy of the internal storage.
             * @return Result<StorageType>
             */
            Result<StorageType> serialize() const {
                return derived().impl_serialize();
            }
            /**
             * @brief Reconstructs the frame from raw wire protocol data.
             * The implementation in the derived class must validate the input data and populate the internal storage.
             * @param data
             * @return Result<Status>
             */
            Result<Status> deserialize(const StorageType& data) {
                return derived().impl_deserialize(data);
            }
            /**
             * @brief Generic validation method for the entire frame.
             *
             * @return Result<Status>
             */
            Result<Status> validate() const {
                return derived().impl_validate();
            }

            /**
             * @brief Dumps the frame's content to the provided output stream in a human-readable format.
             * This method is useful for debugging and logging purposes.
             * It will output all relevant fields of the frame, including ID, payload, type, and any other metadata.
             * The format of the output is designed to be easily interpretable by developers.
             * @param os The output stream to which the frame's content will be written.
             * @return Result<StorageType> containing the raw storage data if successful, or an error otherwise.
             */
            Result<StorageType> dump(std::ostream& os) const {
                return derived().impl_dump(os);
            }
    };
} // namespace USBCANBridge
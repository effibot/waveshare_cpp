#pragma once

#include <boost/core/span.hpp>

#include "../protocol.hpp"
#include "../result.hpp"
#include "../frame_traits.hpp"

using namespace boost;

namespace USBCANBridge {


    template<typename Derived>
    class Core {
        static_assert(std::is_base_of_v<Core<Derived>, Derived>,
            "Derived must inherit from Core<Derived>");
        using traits = frame_traits_t<Derived>;
        using layout = layout_t<Derived>;
        using storage = storage_t<Derived>;

        protected:

            // * CRTP helper to access derived class methods
            Derived& derived() {
                return static_cast<Derived&>(*this);
            }
            // * CRTP helper to access derived class methods (const version)
            const Derived& derived() const {
                return static_cast<const Derived&>(*this);
            }
            // * Storage for the frame data
            alignas(4) mutable storage frame_storage_{};

        public:

            // * Default constructor and destructor
            Core() = default;
            ~Core() = default;

            // === Common Frame Methods ===

            /**
             * @brief Serialize the frame to a byte array.
             * Convert the frame into a read-only byte array for transmission.
             * @note This is common for all frame types and does't call derived().
             * @return Result<span<const std::byte> > A span representing the serialized frame data.
             */
            Result<span<const std::byte> > serialize() const {
                return Result<span<const std::byte> >(span<const std::byte>(frame_storage_));
            }

            /**
             * @brief Deserialize the frame from a byte array.
             * @param data A span representing the serialized frame data.
             * @return Result<Status> Status::SUCCESS on success, or an error status on failure.
             * @note This calls derived().deserialize() for frame-specific deserialization.
             */
            Result<Status> deserialize(span<const std::byte> data) {
                return derived().impl_deserialize(data);
            }

            /**
             * @brief Validate the frame.
             * @return Result<Status> Status::SUCCESS if the frame is valid, or an error status if invalid.
             * @note This calls derived().validate() for frame-specific validation.
             */
            Result<Status> validate() const {
                return derived().impl_validate();
            }
            /**
             * @brief Print the frame in a human-readable format.
             * @return Result<std::string> A string representation of the frame.
             * @note This calls derived().to_string() for frame-specific string conversion.
             */
            Result<std::string> to_string() const {
                return dump_frame<Derived>(frame_storage_);
            }
            /**
             * @brief Get the size of the frame in bytes.
             * @note If the frame has a fixed size, this returns that size. If variable, it calls derived() to compute the size.
             * @return Result<std::size_t> The size of the frame in bytes.
             */
            Result<std::size_t> size() const {
                if constexpr (!traits::is_variable_size) {
                    return Result<std::size_t>(traits::FRAME_SIZE);
                }
                return derived().impl_size();

            }

            // === Protocol-Specific Methods ===
            /**
             * @brief Get the Type byte of the frame.
             * @return Result<Type> The Type byte of the frame.
             * @note This calls derived().get_type() for frame-specific type retrieval.
             */
            Result<Type> get_type() const {
                return derived().impl_get_type();
            }
            /**
             * @brief Set the Type byte of the frame.
             * @param type The Type byte to set.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             * @note This calls derived().set_type() for frame-specific type setting.
             */
            Result<void> set_type(Type type) {
                return derived().impl_set_type(type);
            }
            
    };
}
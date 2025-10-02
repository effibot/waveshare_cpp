#pragma once
#include "../result.hpp"
#include "../frame_traits.hpp"
#include <cstddef>
#include <cstdint>
#include <numeric>



namespace USBCANBridge {
    template<typename Derived>
    class ChecksumInterface {
        // * Ensure that Derived is VariableFrame or FixedFrame, which have checksums
        static_assert(has_checksum_v<Derived>,
            "Derived must be a frame type with checksum");

        protected:
            const Derived& derived() const {
                return static_cast<const Derived&>(*this);
            }
            Derived& derived() {
                return static_cast<Derived&>(*this);
            }
        private:
            mutable bool dirty_ = true;

        protected:
            void mark_dirty() const {
                dirty_ = true;
            }
            void mark_clean() const {
                dirty_ = false;
            }
            bool is_dirty() const {
                return dirty_;
            }
            // * Prevent this class from being instantiated directly
            ChecksumInterface() {
                static_assert(!std::is_same_v<Derived, ChecksumInterface>,
                    "ChecksumInterface cannot be instantiated directly");
            }

        public:
            /**
             * @brief Compute the checksum of the frame, given its internal state.
             * The algorithm performs a sum of the interested bytes and takes the lowest 8 bits.
             * @note The checksum is computed over the fields 2 to 18 (inclusive) of Derived (which can be FixedFrame or ConfigFrame).
             * The corresponding fields are:
             * [Type][FrameType][FrameFormat][ID1]...[ID4][DLC][Data0]...[Data7][Reserved]
             * @return The computed checksum value.
             */
            template<typename T = Derived>
            std::enable_if_t<has_checksum_v<T>, Result<std::byte> >
            compute_checksum() const {
                // Get a read-only view of the frame's raw data
                auto raw_data = this->derived().serialize();
                if (!raw_data) {
                    return raw_data.error();
                }
                const auto& data = raw_data.value();
                // Ensure the data length is as expected
                if (data.size() < this->derived().size().value()) {
                    return Result<std::byte>::error(Status::WBAD_LENGTH);
                }
                // Calculate checksum over the specified range using std::accumulate for efficiency
                size_t start_index = this->derived().traits::TYPE_OFFSET;
                size_t end_index = this->derived().traits::CHECKSUM_OFFSET;
                uint16_t sum = std::accumulate(data.begin() + start_index, data.begin() + end_index,
                    uint16_t(0),
                    [](uint16_t acc, std::byte b) {
                        return acc + static_cast<uint8_t>(b);
                    });
                return Result<std::byte>::success(static_cast<std::byte>(sum & 0xFF));
            }
            /**
             * @brief Check if the stored checksum matches the computed checksum.
             * This method verifies the integrity of the frame by comparing the stored checksum
             * with a freshly computed checksum based on the current frame data.
             * @return Result<bool> True if the checksums match, false otherwise. Returns an error status on failure.
             */
            template<typename T = Derived>
            std::enable_if_t<has_checksum_v<T>, Result<bool> >
            verify_checksum() const {
                // Get a read-only view of the frame's raw data
                auto raw_data = this->derived().serialize();
                if (!raw_data) {
                    return raw_data.error();
                }
                const auto& data = raw_data.value();
                // Ensure the data length is as expected
                if (data.size() < this->derived().size().value()) {
                    return Result<bool>::error(Status::WBAD_LENGTH);
                }
                // Extract the stored checksum from the frame
                std::byte stored_checksum = data[this->derived().traits::CHECKSUM_OFFSET];
                // Compute the checksum based on the current frame data
                auto computed_checksum_res = this->compute_checksum();
                if (!computed_checksum_res) {
                    return computed_checksum_res.error();
                }
                std::byte computed_checksum = computed_checksum_res.value();
                // Compare the stored and computed checksums
                return Result<bool>::success(stored_checksum == computed_checksum);
            }
            /**
             * @brief Update the stored checksum to match the computed checksum.
             * This method recalculates the checksum based on the current frame data
             * and updates the stored checksum field accordingly.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             */
            template<typename T = Derived>
            std::enable_if_t<has_checksum_v<T>, Result<void> >
            update_checksum() {
                if (!this->is_dirty()) {
                    return Result<void>::success(); // No need to update if not dirty
                }
                // Compute the checksum based on the current frame data
                auto computed_checksum_res = this->compute_checksum();
                if (!computed_checksum_res) {
                    return computed_checksum_res.error();
                }
                std::byte computed_checksum = computed_checksum_res.value();
                // Update the stored checksum in the frame
                auto raw_data = this->derived().serialize();
                if (!raw_data) {
                    return raw_data.error();
                }
                auto& data = raw_data.value();
                if (data.size() < this->derived().size().value()) {
                    return Result<void>::error(Status::WBAD_LENGTH);
                }
                data[this->derived().traits::CHECKSUM_OFFSET] = computed_checksum;
                this->mark_clean();
                return Result<void>::success();
            }

    };

}
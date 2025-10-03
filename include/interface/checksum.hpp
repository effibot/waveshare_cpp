#pragma once
#include "../template/result.hpp"
#include "../template/frame_traits.hpp"
#include <boost/core/span.hpp>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <vector>

using namespace boost;
namespace USBCANBridge {

    template<typename Frame>
    class ChecksumInterface {
        // * Ensure that Frame is VariableFrame or FixedFrame, which have checksums
        static_assert(has_checksum_v<Frame>,
            "Frame must be a frame type with checksum");


        private:
            const Frame* frame_;
            mutable bool dirty_ = true;

            const Frame* get_frame() const {
                return frame_;
            }

            /**
             * @brief Mark the stored checksum as clean (i.e., up-to-date).
             * This should be called whenever the checksum is updated.
             */
            void mark_clean() const {
                dirty_ = false;
            }

            /**
             * @brief Core logic to compute checksum from raw data.
             * @param data The raw data to compute checksum from
             * @param start_index Starting index for checksum calculation
             * @param end_index Ending index for checksum calculation
             * @return The computed checksum value
             */
            static Result<std::byte> compute_checksum_impl(
                span<const std::byte> data,
                size_t start_index,
                size_t end_index) {

                if (data.size() < end_index) {
                    return Result<std::byte>::error(Status::WBAD_LENGTH);
                }

                uint16_t sum = std::accumulate(data.begin() + start_index, data.begin() + end_index,
                    uint16_t(0),
                    [](uint16_t acc, std::byte b) {
                        return acc + static_cast<uint8_t>(b);
                    });
                return Result<std::byte>::success(static_cast<std::byte>(sum & 0xFF));
            }            /**
                          * @brief Core logic to verify checksum from raw data.
                          * @param data The raw data to verify checksum for
                          * @param checksum_index Index where the stored checksum is located
                          * @param start_index Starting index for checksum calculation
                          * @param end_index Ending index for checksum calculation
                          * @return True if checksums match, false otherwise
                          */
            static Result<bool> verify_checksum_impl(
                span<const std::byte> data,
                size_t checksum_index,
                size_t start_index,
                size_t end_index) {

                if (data.size() <= checksum_index) {
                    return Result<bool>::error(Status::WBAD_LENGTH);
                }

                std::byte stored_checksum = data[checksum_index];
                auto computed_checksum_res = compute_checksum_impl(data, start_index, end_index);
                if (!computed_checksum_res) {
                    return Result<bool>::error(computed_checksum_res.error());
                }

                return Result<bool>::success(stored_checksum == computed_checksum_res.value());
            }

            /**
             * @brief Core logic to update checksum in raw data.
             * @param data The raw data to update checksum for
             * @param checksum_index Index where the checksum should be stored
             * @param start_index Starting index for checksum calculation
             * @param end_index Ending index for checksum calculation
             * @return Success status or error
             */
            static Result<void> update_checksum_impl(
                span<std::byte> data,
                size_t checksum_index,
                size_t start_index,
                size_t end_index) {

                if (data.size() <= checksum_index) {
                    return Result<void>::error(Status::WBAD_LENGTH);
                }

                auto computed_checksum_res = compute_checksum_impl(data, start_index, end_index);
                if (!computed_checksum_res) {
                    return Result<void>::error(computed_checksum_res.error());
                }

                data[checksum_index] = computed_checksum_res.value();
                return Result<void>::success();
            }

        public:
            /**
             * @brief Construct a ChecksumInterface for the given frame.
             * @param frame The frame to associate with this ChecksumInterface.
             */
            explicit ChecksumInterface(const Frame& frame) : frame_(&frame) {
                mark_dirty();
            }
            /**
             * @brief Check if the stored checksum is dirty (i.e., needs to be recomputed).
             *
             * @return True if the checksum is dirty, false otherwise.
             */
            bool is_dirty() const {
                return dirty_;
            }
            /**
             * @brief Mark the stored checksum as dirty (i.e., needing recomputation).
             * This should be called whenever the frame data changes.
             */
            void mark_dirty() const {
                dirty_ = true;
            }


            /**
             * @brief Compute the checksum of the frame, given its internal state.
             * The algorithm performs a sum of the interested bytes and takes the lowest 8 bits.
             * @note The checksum is computed over the fields 2 to 18 (inclusive) of Derived (which can be FixedFrame or ConfigFrame).
             * The corresponding fields are:
             * [Type][FrameType][FrameFormat][ID1]...[ID4][DLC][Data0]...[Data7][Reserved]
             * @return The computed checksum value.
             */
            template<typename T = Frame>
            std::enable_if_t<has_checksum_v<T>, Result<std::byte> >
            compute_checksum() const {
                auto raw_data = this->get_frame()->serialize();
                if (!raw_data) {
                    return raw_data.error();
                }

                return compute_checksum_impl(
                    raw_data.value(),
                    this->get_frame()->layout::TYPE_OFFSET,
                    this->get_frame()->layout::CHECKSUM_OFFSET
                );
            }

            /**
             * @brief Compute the checksum for external data with frame type inference.
             * @param data The raw data to compute checksum from
             * @return The computed checksum value
             */
            template<typename T = Frame>
            static std::enable_if_t<has_checksum_v<T>, Result<std::byte> >
            compute_checksum(span<const std::byte> data) {
                // Use Frame's traits for offset information
                return compute_checksum_impl(
                    data,
                    Frame::layout::TYPE_OFFSET,
                    Frame::layout::CHECKSUM_OFFSET
                );
            }

            /**
             * @brief Compute the checksum for external data with explicit offsets.
             * @param data The raw data to compute checksum from
             * @param start_offset Starting offset for checksum calculation
             * @param end_offset Ending offset for checksum calculation
             * @return The computed checksum value
             */
            static Result<std::byte> compute_checksum(
                span<const std::byte> data,
                size_t start_offset,
                size_t end_offset) {
                return compute_checksum_impl(data, start_offset, end_offset);
            }
            /**
             * @brief Check if the stored checksum matches the computed checksum.
             * This method verifies the integrity of the frame by comparing the stored checksum
             * with a freshly computed checksum based on the current frame data.
             * @return Result<bool> True if the checksums match, false otherwise. Returns an error status on failure.
             */
            template<typename T = Frame>
            std::enable_if_t<has_checksum_v<T>, Result<bool> >
            verify_checksum() const {
                auto raw_data = this->get_frame()->serialize();
                if (!raw_data) {
                    return raw_data.error();
                }

                return verify_checksum_impl(
                    raw_data.value(),
                    this->get_frame()->layout::CHECKSUM_OFFSET,
                    this->get_frame()->layout::TYPE_OFFSET,
                    this->get_frame()->layout::CHECKSUM_OFFSET
                );
            }

            /**
             * @brief Verify checksum for external data with frame type inference.
             * @param data The raw data to verify checksum for
             * @return True if checksums match, false otherwise
             */
            template<typename T = Frame>
            static std::enable_if_t<has_checksum_v<T>, Result<bool> >
            verify_checksum(span<const std::byte> data) {
                return verify_checksum_impl(
                    data,
                    Frame::layout::CHECKSUM_OFFSET,
                    Frame::layout::TYPE_OFFSET,
                    Frame::layout::CHECKSUM_OFFSET
                );
            }

            /**
             * @brief Verify checksum for external data with explicit offsets.
             * @param data The raw data to verify checksum for
             * @param checksum_offset Offset where the stored checksum is located
             * @param start_offset Starting offset for checksum calculation
             * @param end_offset Ending offset for checksum calculation
             * @return True if checksums match, false otherwise
             */
            static Result<bool> verify_checksum(
                span<const std::byte> data,
                size_t checksum_offset,
                size_t start_offset,
                size_t end_offset) {
                return verify_checksum_impl(data, checksum_offset, start_offset, end_offset);
            }
            /**
             * @brief Update the stored checksum to match the computed checksum.
             * This method recalculates the checksum based on the current frame data
             * and updates the stored checksum field accordingly.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             */
            template<typename T = Frame>
            std::enable_if_t<has_checksum_v<T>, Result<void> >
            update_checksum() {
                if (!this->is_dirty()) {
                    return Result<void>::success(); // No need to update if not dirty
                }

                auto raw_data = this->get_frame()->serialize();
                if (!raw_data) {
                    return raw_data.error();
                }
                auto& data = raw_data.value();

                auto result = update_checksum_impl(
                    data,
                    this->get_frame()->layout::CHECKSUM_OFFSET,
                    this->get_frame()->layout::TYPE_OFFSET,
                    this->get_frame()->layout::CHECKSUM_OFFSET
                );

                if (result) {
                    this->mark_clean();
                }

                return result;
            }

            /**
             * @brief Update checksum for external data with frame type inference.
             * @param data The raw data to update checksum for
             * @return Success status or error
             */
            template<typename T = Frame>
            static std::enable_if_t<has_checksum_v<T>, Result<void> >
            update_checksum(span<std::byte> data) {
                return update_checksum_impl(
                    data,
                    Frame::layout::CHECKSUM_OFFSET,
                    Frame::layout::TYPE_OFFSET,
                    Frame::layout::CHECKSUM_OFFSET
                );
            }

            /**
             * @brief Update checksum for external data with explicit offsets.
             * @param data The raw data to update checksum for
             * @param checksum_offset Offset where the checksum should be stored
             * @param start_offset Starting offset for checksum calculation
             * @param end_offset Ending offset for checksum calculation
             * @return Success status or error
             */
            static Result<void> update_checksum(
                span<std::byte> data,
                size_t checksum_offset,
                size_t start_offset,
                size_t end_offset) {
                return update_checksum_impl(data, checksum_offset, start_offset, end_offset);
            }

    };

}
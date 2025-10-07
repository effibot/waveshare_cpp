#pragma once
#include "../template/result.hpp"
#include "../template/frame_traits.hpp"
#include <numeric>

using namespace boost;
namespace USBCANBridge {

    template<typename Frame>
    class ChecksumInterface {
        // * Ensure that Frame is VariableFrame or FixedFrame, which have checksums
        static_assert(has_checksum_v<Frame>,
            "Frame must be a frame type with checksum");

        private:
            // * Reference to the associated frame
            Frame& frame_;

            Frame& get_frame() {
                return frame_;
            }

            const Frame& get_frame() const {
                return frame_;
            }

            // * Dirty flag to track if checksum needs recomputation
            mutable bool dirty_ = true;

            // * CRTP helper to derive checksum-related offsets and sizes
            const layout_t<Frame>& layout_;
            constexpr static std::size_t CHECKSUM = layout_t<Frame>::CHECKSUM;
            constexpr static std::size_t CHECKSUM_START = layout_t<Frame>::TYPE;
            constexpr static std::size_t CHECKSUM_END = layout_t<Frame>::CHECKSUM;  // ? just to use different naming for clarity

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
             * @return The computed checksum value
             * @throws std::runtime_error if data size is insufficient
             */
            static std::byte compute_checksum_impl(const storage_t<Frame> data) {

                if (data.size() <= CHECKSUM_END) {
                    throw std::runtime_error("Checksum calculation: invalid data length");
                }

                uint16_t sum = std::accumulate(data.begin() + CHECKSUM_START,
                    data.begin() + CHECKSUM_END,
                    uint16_t(0),
                    [](uint16_t acc, std::byte b) {
                        return acc + static_cast<uint8_t>(b);
                    });
                return static_cast<std::byte>(sum & 0xFF);
            }

            /**
             * @brief Core logic to verify checksum from raw data.
             * @param data The raw data to verify checksum for
             * @return True if checksums match, false otherwise
             * @throws std::runtime_error if data size is insufficient
             */
            static bool verify_checksum_impl(const storage_t<Frame> data) {

                if (data.size() <= CHECKSUM) {
                    throw std::runtime_error("Checksum verification: invalid data length");
                }

                std::byte stored_checksum = data[CHECKSUM];
                std::byte computed_checksum = compute_checksum_impl(data);

                return stored_checksum == computed_checksum;
            }

            /**
             * @brief Core logic to update checksum in raw data.
             * @param data The raw data to update checksum for
             * @throws std::runtime_error if data size is insufficient
             */
            static void update_checksum_impl(storage_t<Frame>& data) {

                if (data.size() <= CHECKSUM) {
                    throw std::runtime_error("Checksum update: invalid data length");
                }

                std::byte computed_checksum = compute_checksum_impl(data);
                data[CHECKSUM] = computed_checksum;
            }

        public:
            /**
             * @brief Construct a ChecksumInterface for the given frame.
             * @param frame The frame to associate with this ChecksumInterface.
             */
            ChecksumInterface(Frame& frame) : frame_(frame), layout_(frame.get_layout()) {
                mark_dirty();
            }

            ~ChecksumInterface() = default;

            // === Utility Methods to Set/Unset Dirty bit ===
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

            // === Concrete Checksum Operations ===
            /**
             * @brief Compute the checksum of the frame, given its internal state.
             * The algorithm performs a sum of the interested bytes and takes the lowest 8 bits.
             * @note The checksum is computed over the fields 2 to 18 (inclusive) of Derived (which can be FixedFrame or ConfigFrame).
             * The corresponding fields are:
             * [Type][CANVersion][Format][ID1]...[ID4][DLC][Data0]...[Data7][Reserved]
             * @return The computed checksum value.
             */
            template<typename T = Frame>
            std::enable_if_t<has_checksum_v<T>, std::byte>
            compute_checksum() const {
                const storage_t<Frame>& raw_data = this->get_frame().get_storage();

                return compute_checksum_impl(raw_data);
            }

            /**
             * @brief Compute the checksum for external data with frame type inference.
             * @param data The raw data to compute checksum from
             * @return The computed checksum value
             */
            template<typename T = Frame>
            static std::enable_if_t<has_checksum_v<T>, std::byte>
            compute_checksum(const storage_t<Frame> data) {
                // Use Frame's traits for offset information
                return compute_checksum_impl(data);
            }

            /**
             * @brief Check if the stored checksum matches the computed checksum.
             * This method verifies the integrity of the frame by comparing the stored checksum
             * with a freshly computed checksum based on the current frame data.
             * @return True if the checksums match, false otherwise.
             */
            template<typename T = Frame>
            std::enable_if_t<has_checksum_v<T>, bool>
            verify_checksum() const {
                const storage_t<Frame>& raw_data = this->get_frame().get_storage();

                return verify_checksum_impl(raw_data);
            }

            /**
             * @brief Verify checksum for external data with frame type inference.
             * @param data The raw data to verify checksum for
             * @return True if checksums match, false otherwise
             */
            template<typename T = Frame>
            static std::enable_if_t<has_checksum_v<T>, bool>
            verify_checksum(const storage_t<Frame>& data) {
                return verify_checksum_impl(data);
            }

            /**
             * @brief Update the stored checksum to match the computed checksum.
             * This method recalculates the checksum based on the current frame data
             * and updates the stored checksum field accordingly.
             * If the checksum is not marked as dirty, this method does nothing.
             * @note After updating, the checksum is marked as clean.
             */
            template<typename T = Frame>
            std::enable_if_t<has_checksum_v<T>, void>
            update_checksum() {
                if (!this->is_dirty()) {
                    return; // No need to update if not dirty
                }

                storage_t<Frame>& raw_data = this->get_frame().get_storage();

                update_checksum_impl(raw_data);

                this->mark_clean();
            }

            /**
             * @brief Update checksum for external data with frame type inference.
             * @param data The raw data to update checksum for
             */
            template<typename T = Frame>
            static std::enable_if_t<has_checksum_v<T>, void>
            update_checksum(storage_t<Frame>& data) {
                update_checksum_impl(data);
            }
    };

}
/**
 * @file span_compat.hpp
 * @brief C++17 compatible span-like utility for byte array access.
 *
 * This file provides a simple span-like class that works with C++17,
 * providing a safe and convenient interface for working with contiguous
 * memory ranges without the need for C++20's std::span.
 *
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @version 1.0
 * @date 2025-09-29
 * @copyright Copyright (c) 2025
 */

#pragma once

#include <array>
#include <vector>
#include <cstddef>
#include <type_traits>

namespace USBCANBridge {

/**
 * @brief Simple span-like class for C++17 compatibility.
 *
 * Provides a non-owning view over a contiguous sequence of objects.
 * This is a simplified version of C++20's std::span that works with C++17.
 *
 * @tparam T Element type
 */
    template<typename T>
    class span {
        private:
            T* data_;
            std::size_t size_;

        public:
            using element_type = T;
            using value_type = std::remove_cv_t<T>;
            using size_type = std::size_t;
            using pointer = T*;
            using const_pointer = const T*;
            using reference = T&;
            using const_reference = const T&;
            using iterator = T*;
            using const_iterator = const T*;

            /**
             * @brief Construct span from pointer and size.
             */
            constexpr span(T* ptr, std::size_t count) noexcept
                : data_(ptr), size_(count) {
            }

            /**
             * @brief Construct span from std::array.
             */
            template<std::size_t N>
            constexpr span(std::array<T, N>& arr) noexcept
                : data_(arr.data()), size_(N) {
            }

            /**
             * @brief Construct span from const std::array.
             */
            template<std::size_t N>
            constexpr span(const std::array<T, N>& arr) noexcept
                : data_(arr.data()), size_(N) {
            }

            /**
             * @brief Construct span from std::vector.
             */
            constexpr span(std::vector<std::remove_const_t<T> >& vec) noexcept
                : data_(vec.data()), size_(vec.size()) {
            }

            /**
             * @brief Construct span from const std::vector.
             */
            constexpr span(const std::vector<std::remove_const_t<T> >& vec) noexcept
                : data_(vec.data()), size_(vec.size()) {
            }

            /**
             * @brief Default constructor creates empty span.
             */
            constexpr span() noexcept : data_(nullptr), size_(0) {
            }

            // Element access
            constexpr T* data() const noexcept {
                return data_;
            }
            constexpr std::size_t size() const noexcept {
                return size_;
            }
            constexpr bool empty() const noexcept {
                return size_ == 0;
            }

            constexpr reference operator[](std::size_t idx) const noexcept {
                return data_[idx];
            }

            constexpr reference front() const noexcept {
                return data_[0];
            }
            constexpr reference back() const noexcept {
                return data_[size_ - 1];
            }

            // Iterators
            constexpr iterator begin() const noexcept {
                return data_;
            }
            constexpr iterator end() const noexcept {
                return data_ + size_;
            }
            constexpr const_iterator cbegin() const noexcept {
                return data_;
            }
            constexpr const_iterator cend() const noexcept {
                return data_ + size_;
            }

            /**
             * @brief Create subspan from offset.
             */
            constexpr span<T> subspan(std::size_t offset) const noexcept {
                return span<T>(data_ + offset, size_ - offset);
            }

            /**
             * @brief Create subspan from offset with count.
             */
            constexpr span<T> subspan(std::size_t offset, std::size_t count) const noexcept {
                return span<T>(data_ + offset, count);
            }

            /**
             * @brief Get first N elements.
             */
            template<std::size_t Count>
            constexpr span<T> first() const noexcept {
                return span<T>(data_, Count);
            }

            /**
             * @brief Get first count elements.
             */
            constexpr span<T> first(std::size_t count) const noexcept {
                return span<T>(data_, count);
            }

            /**
             * @brief Get last N elements.
             */
            template<std::size_t Count>
            constexpr span<T> last() const noexcept {
                return span<T>(data_ + size_ - Count, Count);
            }

            /**
             * @brief Get last count elements.
             */
            constexpr span<T> last(std::size_t count) const noexcept {
                return span<T>(data_ + size_ - count, count);
            }
    };

// Deduction guides for C++17
    template<typename T, std::size_t N>
    span(std::array<T, N>&)->span<T>;

    template<typename T, std::size_t N>
    span(const std::array<T, N>&)->span<const T>;

    template<typename T>
    span(std::vector<T>&)->span<T>;

    template<typename T>
    span(const std::vector<T>&)->span<const T>;

    template<typename T>
    span(T*, std::size_t)->span<T>;

} // namespace USBCANBridge
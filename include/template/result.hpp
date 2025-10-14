/**
 * @file result.hpp
 * @brief Enhanced Result type with automatic error chaining for better error reporting.
 * @version 3.0
 * @date 2025-10-03
 * @copyright Copyright (c) 2025
 */

#pragma once
#include "../enums/error.hpp"
#include <string>
#include <variant>
#include <vector>

namespace waveshare {

/**
 * @brief Enhanced Result type with automatic error chaining.
 *
 * Wraps a value of type T or an error status with automatic error context chaining.
 * When an error is propagated through error(), it automatically builds a call stack
 * for better debugging and error reporting.
 *
 * @tparam T The type of the value being returned.
 */
    template<typename T>
    class Result {
        private:
            std::variant<T, Status> value_or_error_;
            std::vector<std::string> error_chain_;

        public:
            // Default constructor with error status
            Result() : value_or_error_(Status::UNKNOWN) {
            }

            // Existing interface preserved for compatibility
            bool ok() const {
                return std::holds_alternative<T>(value_or_error_);
            }

            bool fail() const {
                return !ok();
            }

            explicit operator bool() const {
                return ok();
            }

            bool operator!() const {
                return !ok();
            }

            // Value access (throws if error)
            const T& value() const {
                return std::get<T>(value_or_error_);
            }

            T& value() {
                return std::get<T>(value_or_error_);
            }

            // Error access
            Status error() const {
                return fail() ? std::get<Status>(value_or_error_) : Status::SUCCESS;
            }

            // Enhanced error reporting with full chain
            std::string describe() const {
                if (ok()) return "Success";

                std::string result = "Error";
                if (!error_chain_.empty()) {
                    result += " [";
                    for (size_t i = 0; i < error_chain_.size(); ++i) {
                        if (i > 0) result += " -> ";
                        result += error_chain_[i];
                    }
                    result += "]";
                }
                return result;
            }

            std::string to_string() const {
                return describe();
            }

            // Get the full error chain
            const std::vector<std::string>& error_chain() const {
                return error_chain_;
            }

            // Factory methods with context
            static Result success(T val, const std::string& op = "") {
                Result r;
                r.value_or_error_ = std::move(val);
                if (!op.empty()) {
                    r.error_chain_.push_back(op);
                }
                return r;
            }

            static Result error(Status status, const std::string& op = "") {
                Result r;
                r.value_or_error_ = status;
                if (!op.empty()) {
                    r.error_chain_.push_back(op);
                }
                return r;
            }

            // Automatic error chaining - propagate error from another Result
            template<typename U>
            static Result error(const Result<U>& failed_result, const std::string& op = "") {
                Result r;
                r.value_or_error_ = failed_result.error();
                r.error_chain_ = failed_result.error_chain();
                if (!op.empty()) {
                    r.error_chain_.push_back(op);
                }
                return r;
            }

            // Chaining operations (monadic interface)
            template<typename F>
            auto and_then(F&& func) -> Result<std::invoke_result_t<F, T> > {
                using ReturnType = std::invoke_result_t<F, T>;
                if (fail()) {
                    return Result<ReturnType>::error(*this, "");
                }
                return func(value());
            }
    };

/**
 * @brief Specialization for Result<void> - operations that don't return values.
 */
    template<>
    class Result<void> {
        private:
            Status status_ = Status::SUCCESS;
            std::vector<std::string> error_chain_;

        public:
            // Existing interface preserved for compatibility
            bool ok() const {
                return status_ == Status::SUCCESS;
            }

            bool fail() const {
                return !ok();
            }

            explicit operator bool() const {
                return ok();
            }

            bool operator!() const {
                return !ok();
            }

            // Error access
            Status error() const {
                return status_;
            }

            // Enhanced error reporting with full chain
            std::string describe() const {
                if (ok()) return "Success";

                std::string result = "Error";
                if (!error_chain_.empty()) {
                    result += " [";
                    for (size_t i = 0; i < error_chain_.size(); ++i) {
                        if (i > 0) result += " -> ";
                        result += error_chain_[i];
                    }
                    result += "]";
                }
                return result;
            }

            std::string to_string() const {
                return describe();
            }

            // Get the full error chain
            const std::vector<std::string>& error_chain() const {
                return error_chain_;
            }

            // Factory methods with context
            static Result success(const std::string& op = "") {
                Result r;
                r.status_ = Status::SUCCESS;
                if (!op.empty()) {
                    r.error_chain_.push_back(op);
                }
                return r;
            }

            static Result error(Status status, const std::string& op = "") {
                Result r;
                r.status_ = status;
                if (!op.empty()) {
                    r.error_chain_.push_back(op);
                }
                return r;
            }

            // Automatic error chaining - propagate error from another Result
            template<typename U>
            static Result error(const Result<U>& failed_result, const std::string& op = "") {
                Result r;
                r.status_ = failed_result.error();
                r.error_chain_ = failed_result.error_chain();
                if (!op.empty()) {
                    r.error_chain_.push_back(op);
                }
                return r;
            }
    };

}// namespace USBCANBridge

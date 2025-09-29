/**
 * @file result.hpp
 * @brief Enhanced Result type with operation context for better error reporting.
 * @version 2.0
 * @date 2025-09-29
 * @copyright Copyright (c) 2025
 */

#pragma once
#include "error.hpp"
#include <string>
#include <variant>

namespace USBCANBridge {

/**
 * @brief Enhanced Result type with operation context.
 *
 * Wraps a value of type T or an error status with optional operation context
 * for better error reporting and debugging. Maintains the existing interface
 * while adding enhanced error descriptions.
 *
 * @tparam T The type of the value being returned.
 */
    template<typename T>
    class Result {
        private:
            std::variant<T, Status> value_or_error_;
            std::string operation_context_;

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

            // Enhanced error reporting
            std::string describe() const {
                if (ok()) return "Success";
                const auto status = std::get<Status>(value_or_error_);
                return operation_context_.empty() ?
                       "Error" : // Could use status_to_string(status) if available
                       operation_context_ + ": Error";
            }

            std::string to_string() const {
                return describe();
            }

            // Factory methods with context
            static Result success(T val, const std::string& op = "") {
                Result r;
                r.value_or_error_ = std::move(val);
                r.operation_context_ = op;
                return r;
            }

            static Result error(Status status, const std::string& op = "") {
                Result r;
                r.value_or_error_ = status;
                r.operation_context_ = op;
                return r;
            }

            // Chaining operations (monadic interface)
            template<typename F>
            auto and_then(F&& func) -> Result<std::invoke_result_t<F, T> > {
                using ReturnType = std::invoke_result_t<F, T>;
                if (fail()) {
                    return Result<ReturnType>::error(error(), operation_context_);
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
            std::string operation_context_;

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

            // Enhanced error reporting
            std::string describe() const {
                if (ok()) return "Success";
                return operation_context_.empty() ?
                       "Error" :
                       operation_context_ + ": Error";
            }

            std::string to_string() const {
                return describe();
            }

            // Factory methods with context
            static Result success(const std::string& op = "") {
                Result r;
                r.status_ = Status::SUCCESS;
                r.operation_context_ = op;
                return r;
            }

            static Result error(Status status, const std::string& op = "") {
                Result r;
                r.status_ = status;
                r.operation_context_ = op;
                return r;
            }
    };

}// namespace USBCANBridge

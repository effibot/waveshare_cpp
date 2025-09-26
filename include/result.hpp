/**
 * @file result.hpp
 * @author your name (you@domain.com)
 * @brief Result type for function return values.
 * @version 0.1
 * @date 2025-09-21
 *
 */

#pragma once
#include "error.hpp"
#include <string>
#include <system_error>

namespace USBCANBridge {
/**
 * @brief Result type for function return values.
 *
 * Wraps a value of type T and an error code.
 * Provides convenience methods for checking success/failure.
 *
 * @tparam T The type of the value being returned.
 */
	template<typename T>
	struct Result {
		T value;
		std::error_code status;

		// Convenience methods
		bool ok() const {
			return status == Status::SUCCESS;
		}
		bool fail() const {
			return status != Status::SUCCESS;
		}
		explicit operator bool() const {
			return ok();
		}
		bool operator!() const {
			return !ok();
		}
		std::string to_string() {
			return status.message();
		}
		static Result success(T val) {
			return {std::move(val), Status::SUCCESS};
		}
		static Result error(Status err) {
			return {T{}, err};
		}
	};

}// namespace USBCANBridge

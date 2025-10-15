/**
 * @file test_exceptions.cpp
 * @brief Unit tests for Waveshare exception hierarchy
 */

#include <catch2/catch_test_macros.hpp>
#include "../include/exception/waveshare_exception.hpp"

using namespace waveshare;

TEST_CASE("WaveshareException - Basic functionality", "[exception]") {
    SECTION("Base exception stores status and context") {
        try {
            throw WaveshareException(Status::WBAD_CHECKSUM, "test_function");
            REQUIRE(false); // Should not reach here
        } catch (const WaveshareException& e) {
            REQUIRE(e.status() == Status::WBAD_CHECKSUM);
            REQUIRE(e.context() == "test_function");
            REQUIRE(std::string(e.what()).find("Bad checksum") != std::string::npos);
            REQUIRE(std::string(e.what()).find("test_function") != std::string::npos);
        }
    }
}

TEST_CASE("ProtocolException - Protocol validation errors", "[exception]") {
    SECTION("Thrown for WBAD_* status codes") {
        REQUIRE_THROWS_AS(
            throw ProtocolException(Status::WBAD_START, "deserialize"),
            ProtocolException
        );

        REQUIRE_THROWS_AS(
            throw ProtocolException(Status::WBAD_CHECKSUM, "validate"),
            ProtocolException
        );
    }

    SECTION("Is derived from WaveshareException") {
        try {
            throw ProtocolException(Status::WBAD_ID, "set_id");
        } catch (const WaveshareException& e) {
            REQUIRE(e.status() == Status::WBAD_ID);
            REQUIRE(std::string(e.what()).find("Bad CAN ID") != std::string::npos);
        }
    }
}

TEST_CASE("DeviceException - Device I/O errors", "[exception]") {
    SECTION("Thrown for device-related errors") {
        REQUIRE_THROWS_AS(
            throw DeviceException(Status::DNOT_OPEN, "write_bytes"),
            DeviceException
        );

        REQUIRE_THROWS_AS(
            throw DeviceException(Status::DREAD_ERROR, "read_bytes"),
            DeviceException
        );
    }
}

TEST_CASE("TimeoutException - Timeout errors", "[exception]") {
    SECTION("Thrown for timeout status") {
        REQUIRE_THROWS_AS(
            throw TimeoutException(Status::WTIMEOUT, "receive_frame"),
            TimeoutException
        );
    }

    SECTION("Is derived from WaveshareException") {
        try {
            throw TimeoutException(Status::WTIMEOUT, "read_exact");
        } catch (const WaveshareException& e) {
            REQUIRE(e.status() == Status::WTIMEOUT);
            REQUIRE(std::string(e.what()).find("Timeout") != std::string::npos);
        }
    }
}

TEST_CASE("CANException - CAN protocol errors", "[exception]") {
    SECTION("Thrown for CAN-specific errors") {
        REQUIRE_THROWS_AS(
            throw CANException(Status::CAN_SDO_TIMEOUT, "sdo_transfer"),
            CANException
        );
    }
}

TEST_CASE("throw_error - Factory function", "[exception]") {
    SECTION("Throws correct exception type based on status") {
        // Protocol errors
        REQUIRE_THROWS_AS(
            throw_error(Status::WBAD_CHECKSUM, "test"),
            ProtocolException
        );

        REQUIRE_THROWS_AS(
            throw_error(Status::WBAD_LENGTH, "test"),
            ProtocolException
        );

        // Device errors
        REQUIRE_THROWS_AS(
            throw_error(Status::DNOT_FOUND, "test"),
            DeviceException
        );

        REQUIRE_THROWS_AS(
            throw_error(Status::DWRITE_ERROR, "test"),
            DeviceException
        );

        // Timeout
        REQUIRE_THROWS_AS(
            throw_error(Status::WTIMEOUT, "test"),
            TimeoutException
        );

        // CAN errors
        REQUIRE_THROWS_AS(
            throw_error(Status::CAN_SDO_ABORT, "test"),
            CANException
        );
    }
}

TEST_CASE("throw_if_error - Conditional throw", "[exception]") {
    SECTION("Does not throw on SUCCESS") {
        REQUIRE_NOTHROW(
            throw_if_error(Status::SUCCESS, "test")
        );
    }

    SECTION("Throws on error status") {
        REQUIRE_THROWS_AS(
            throw_if_error(Status::WBAD_CHECKSUM, "test"),
            ProtocolException
        );
    }
}

TEST_CASE("Exception hierarchy - Polymorphic catching", "[exception]") {
    SECTION("Can catch derived exceptions as base") {
        bool caught_protocol = false;
        bool caught_device = false;
        bool caught_timeout = false;
        bool caught_base = false;

        try {
            throw ProtocolException(Status::WBAD_ID, "test");
        } catch (const WaveshareException&) {
            caught_protocol = true;
        }

        try {
            throw DeviceException(Status::DNOT_OPEN, "test");
        } catch (const WaveshareException&) {
            caught_device = true;
        }

        try {
            throw TimeoutException(Status::WTIMEOUT, "test");
        } catch (const WaveshareException&) {
            caught_timeout = true;
        }

        try {
            throw WaveshareException(Status::UNKNOWN, "test");
        } catch (const WaveshareException&) {
            caught_base = true;
        }

        REQUIRE(caught_protocol);
        REQUIRE(caught_device);
        REQUIRE(caught_timeout);
        REQUIRE(caught_base);
    }

    SECTION("Can distinguish between exception types") {
        auto throw_and_check = [](Status status) -> std::string {
                try {
                    throw_error(status, "test");
                    return "no_throw";
                } catch (const ProtocolException&) {
                    return "protocol";
                }
                catch (const DeviceException&) {
                    return "device";
                }
                catch (const TimeoutException&) {
                    return "timeout";
                }
                catch (const CANException&) {
                    return "can";
                }
                catch (const WaveshareException&) {
                    return "base";
                }
            };

        REQUIRE(throw_and_check(Status::WBAD_CHECKSUM) == "protocol");
        REQUIRE(throw_and_check(Status::DNOT_OPEN) == "device");
        REQUIRE(throw_and_check(Status::WTIMEOUT) == "timeout");
        REQUIRE(throw_and_check(Status::CAN_SDO_ABORT) == "can");
    }
}

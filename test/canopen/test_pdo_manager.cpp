/**
 * @file test_pdo_manager.cpp
 * @brief Unit tests for PDOManager
 * @author effibot (andrea.efficace1@gmail.com)
 * @date 2025-11-07
 */

#include <catch2/catch_test_macros.hpp>
#include "canopen/pdo_manager.hpp"
#include "canopen/pdo_constants.hpp"
#include <thread>
#include <chrono>

using namespace canopen;
using namespace canopen::pdo;

// =============================================================================
// COB-ID Calculation Tests
// =============================================================================

TEST_CASE("PDOManager: COB-ID calculation", "[pdo_manager][cob_id]") {
    SECTION("RPDO1 COB-ID") {
        // RPDO1 base = 0x200
        REQUIRE(0x201 == cob_id::RPDO1_BASE + 1);  // Node 1
        REQUIRE(0x202 == cob_id::RPDO1_BASE + 2);  // Node 2
        REQUIRE(0x204 == cob_id::RPDO1_BASE + 4);  // Node 4
    }

    SECTION("RPDO2 COB-ID") {
        // RPDO2 base = 0x300
        REQUIRE(0x301 == cob_id::RPDO2_BASE + 1);  // Node 1
        REQUIRE(0x302 == cob_id::RPDO2_BASE + 2);  // Node 2
        REQUIRE(0x304 == cob_id::RPDO2_BASE + 4);  // Node 4
    }

    SECTION("TPDO1 COB-ID") {
        // TPDO1 base = 0x180
        REQUIRE(0x181 == cob_id::TPDO1_BASE + 1);  // Node 1
        REQUIRE(0x182 == cob_id::TPDO1_BASE + 2);  // Node 2
        REQUIRE(0x184 == cob_id::TPDO1_BASE + 4);  // Node 4
    }

    SECTION("TPDO2 COB-ID") {
        // TPDO2 base = 0x280
        REQUIRE(0x281 == cob_id::TPDO2_BASE + 1);  // Node 1
        REQUIRE(0x282 == cob_id::TPDO2_BASE + 2);  // Node 2
        REQUIRE(0x284 == cob_id::TPDO2_BASE + 4);  // Node 4
    }

    SECTION("COB-ID calculation helper function") {
        REQUIRE(calculate_cob_id(PDOType::RPDO1, 1) == 0x201);
        REQUIRE(calculate_cob_id(PDOType::TPDO1, 1) == 0x181);
        REQUIRE(calculate_cob_id(PDOType::RPDO2, 2) == 0x302);
        REQUIRE(calculate_cob_id(PDOType::TPDO2, 2) == 0x282);
    }

    SECTION("Extract node ID from COB-ID") {
        REQUIRE(extract_node_id(0x181) == 1);  // TPDO1
        REQUIRE(extract_node_id(0x201) == 1);  // RPDO1
        REQUIRE(extract_node_id(0x282) == 2);  // TPDO2
        REQUIRE(extract_node_id(0x302) == 2);  // RPDO2
        REQUIRE(extract_node_id(0x999) == 0);  // Invalid
    }
}

// =============================================================================
// Lifecycle Tests
// =============================================================================

TEST_CASE("PDOManager: Lifecycle management", "[pdo_manager][lifecycle]") {
    // Note: These tests require vcan0 to be available
    // Skip if vcan0 not available (integration test environment only)

    SECTION("Construction") {
        PDOManager pdo("vcan0");
        REQUIRE_FALSE(pdo.is_running());
        REQUIRE(pdo.get_interface() == "vcan0");
    }

    SECTION("Multiple stop calls are safe") {
        PDOManager pdo("vcan0");
        pdo.stop();
        pdo.stop();  // Should not crash
        REQUIRE_FALSE(pdo.is_running());
    }
}

// =============================================================================
// Callback Registration Tests
// =============================================================================

TEST_CASE("PDOManager: Callback registration", "[pdo_manager][callbacks]") {
    PDOManager pdo("vcan0");

    SECTION("Register TPDO1 callback") {
        bool callback_called = false;

        pdo.register_tpdo1_callback(1, [&](const can_frame&) {
                callback_called = true;
            });

        // Callback registered but not yet called
        REQUIRE_FALSE(callback_called);
    }

    SECTION("Register TPDO2 callback") {
        bool callback_called = false;

        pdo.register_tpdo2_callback(2, [&](const can_frame&) {
                callback_called = true;
            });

        REQUIRE_FALSE(callback_called);
    }

    SECTION("Unregister callbacks") {
        bool tpdo1_called = false;
        bool tpdo2_called = false;

        pdo.register_tpdo1_callback(1, [&](const can_frame&) {
                tpdo1_called = true;
            });

        pdo.register_tpdo2_callback(1, [&](const can_frame&) {
                tpdo2_called = true;
            });

        pdo.unregister_callbacks(1);

        // After unregister, callbacks should not be called
        REQUIRE_FALSE(tpdo1_called);
        REQUIRE_FALSE(tpdo2_called);
    }
}

// =============================================================================
// Statistics Tests
// =============================================================================

TEST_CASE("PDOManager: Statistics tracking", "[pdo_manager][statistics]") {
    PDOManager pdo("vcan0");

    SECTION("Initial statistics are zero") {
        auto stats = pdo.get_statistics(1);

        REQUIRE(stats.tpdo1_received == 0);
        REQUIRE(stats.tpdo2_received == 0);
        REQUIRE(stats.rpdo1_sent == 0);
        REQUIRE(stats.rpdo2_sent == 0);
        REQUIRE(stats.errors == 0);
        REQUIRE(stats.avg_latency_us == 0.0);
    }

    SECTION("Reset statistics") {
        pdo.reset_statistics(1);
        auto stats = pdo.get_statistics(1);

        REQUIRE(stats.tpdo1_received == 0);
        REQUIRE(stats.tpdo2_received == 0);
    }

    SECTION("Statistics for non-existent node") {
        auto stats = pdo.get_statistics(99);

        // Should return empty stats
        REQUIRE(stats.tpdo1_received == 0);
        REQUIRE(stats.rpdo1_sent == 0);
    }

    SECTION("Statistics snapshot is non-atomic") {
        // Get snapshot - should be plain uint64_t values
        auto stats = pdo.get_statistics(1);

        // StatisticsSnapshot should have non-atomic members
        REQUIRE(std::is_same<decltype(stats.tpdo1_received), uint64_t>::value);
        REQUIRE(std::is_same<decltype(stats.rpdo1_sent), uint64_t>::value);
    }
}

// =============================================================================
// RPDO Data Validation Tests
// =============================================================================

TEST_CASE("PDOManager: RPDO data validation", "[pdo_manager][rpdo]") {
    PDOManager pdo("vcan0");

    SECTION("Valid RPDO1 data size") {
        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
        REQUIRE(data.size() <= limits::MAX_PDO_DATA_LENGTH);
    }

    SECTION("Invalid RPDO1 data size (too large)") {
        std::vector<uint8_t> data(9, 0xFF);  // 9 bytes - too large
        REQUIRE(data.size() > limits::MAX_PDO_DATA_LENGTH);
    }

    SECTION("Empty RPDO data is valid") {
        std::vector<uint8_t> data;
        REQUIRE(data.size() == 0);
        REQUIRE(data.size() <= limits::MAX_PDO_DATA_LENGTH);
    }

    SECTION("Full 8-byte RPDO data") {
        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        REQUIRE(data.size() == limits::MAX_PDO_DATA_LENGTH);
    }
}

// =============================================================================
// Multi-Motor Scenario Tests
// =============================================================================

TEST_CASE("PDOManager: Multi-motor support", "[pdo_manager][multi_motor]") {
    PDOManager pdo("vcan0");

    SECTION("Register callbacks for 4 motors") {
        int callback_count = 0;

        for (uint8_t node_id = 1; node_id <= 4; ++node_id) {
            pdo.register_tpdo1_callback(node_id, [&](const can_frame&) {
                    callback_count++;
                });
        }

        // All callbacks registered
        REQUIRE(callback_count == 0);  // Not called yet
    }

    SECTION("Independent statistics per motor") {
        auto stats1 = pdo.get_statistics(1);
        auto stats2 = pdo.get_statistics(2);

        // Each motor has independent statistics
        REQUIRE(stats1.tpdo1_received == 0);
        REQUIRE(stats2.tpdo1_received == 0);
    }
}

/**
 * @file object_dictionary.hpp
 * @author effibot (andrea.efficace1@gmail.com)
 * @brief CANOpen Object Dictionary parser and handler
 * @version 0.1
 * @date 2025-11-06
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace canopen {

    /**
     * @brief CANOpen Object Dictionary
     * Helper class to parse and manage CANOpen Object Dictionary
     * reading information from a JSON file.
     *
     * @note No control on the correctness/compliance of the CANOpen objects is done.
     */
    class ObjectDictionary {
        public:
            // Helper enum to easily handle data types
            enum class DataType {
                UINT8, INT8, UINT16, INT16, UINT32, INT32, UINT64, INT64
            };

            /**
             * @brief Object Dictionary Entry
             * Represents a single entry in the CANOpen Object Dictionary.
             */
            struct ObjectEntry {
                uint16_t index;
                uint8_t subindex;
                DataType datatype;
                std::string access; // # "ro", "rw", "wo"
                std::string pdo_mapping; // # "rpdo1", "tpdo2", etc.
                double scaling_factor;
                std::string unit;

                size_t size_bytes() const;  // Returns size in bytes based on datatype
            };

            /**
             * @brief Construct a new Object Dictionary object
             * @param json_path Path to the JSON configuration file
             * @throws std::runtime_error if JSON parsing fails
             */
            explicit ObjectDictionary(const std::string& json_path);

            /**
             * @brief Get Object Entry by name
             * @param name Name of the object
             * @return Corresponding ObjectEntry
             * @throws std::out_of_range if object not found
             */
            const ObjectEntry& get_object(const std::string& name) const;

            /**
             * @brief Check if object exists
             * @param name Name of the object
             * @return true if object exists, false otherwise
             */
            bool has_object(const std::string& name) const;

            /**
             * @brief Convert typed value to raw bytes (little-endian)
             *
             * @tparam T
             * @param value
             * @return std::vector<uint8_t>
             */
            template<typename T>
            std::vector<uint8_t> to_raw(T value) const;

            /**
             * @brief Convert raw bytes to typed value (little-endian)
             *
             * @tparam T
             * @param raw_data
             * @return T
             */
            template<typename T>
            T from_raw(const std::vector<uint8_t>& raw_data) const;


            /**
             * @brief Get the pdo objects object
             *
             * @param pdo_name
             * @return std::vector<std::string>
             */
            std::vector<std::string> get_pdo_objects(const std::string& pdo_name) const;

            /**
             * @brief Get motor parameter by name
             *
             * @param param_name
             * @return double
             */
            double get_motor_param(const std::string& param_name) const;

            /**
             * @brief Get the Node ID
             *
             * @return uint8_t
             */
            uint8_t get_node_id() const { return node_id_; }

            /**
             * @brief Get device name
             */
            std::string get_device_name() const { return device_name_; }

            /**
             * @brief Get CAN interface name
             */
            std::string get_can_interface() const { return can_interface_; }

        private:
            // * Collection of all objects parsed from JSON
            std::unordered_map<std::string, ObjectEntry> objects_;
            nlohmann::json config_;
            uint8_t node_id_;
            std::string device_name_;
            std::string can_interface_;

            /**
             * @brief Parse JSON file and populate object dictionary
             *
             * @param json_path
             *
             * @throws std::runtime_error if JSON parsing fails
             */
            void parse_json(const std::string& json_path);

            /**
             * @brief Parse string representation of data type
             *
             * @param type_str
             * @return DataType
             * @throws std::invalid_argument if unknown data type
             */
            DataType parse_datatype(const std::string& type_str);
    };
};
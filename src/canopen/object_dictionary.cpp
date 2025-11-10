/**
 * @file object_dictionary.cpp
 * @author effibot (andrea.efficace1@gmail.com)
 * @brief Implementation of the Object Dictionary for CANopen devices
 * @version 0.1
 * @date 2025-11-06
 *
 * @copyright Copyright (c) 2025
 *
 */


#include "canopen/object_dictionary.hpp"
#include <stdexcept>
#include <iostream>
#include <fstream>

namespace canopen {

    ObjectDictionary::ObjectDictionary(const std::string& json_path) {
        try {
            parse_json(json_path);
        }
        catch (const std::exception& e) {
            // If parsing fails, rethrow as runtime_error
            throw std::runtime_error("Failed to parse Object Dictionary JSON: " +
                std::string(e.what()));
        }

    }

    void ObjectDictionary::parse_json(const std::string& json_path) {

        // Load JSON file
        std::ifstream file(json_path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open config file: " + json_path);
        }

        // store JSON content into config_ member and get node ID
        file >> config_;
        node_id_ = config_["node_id"].get<uint8_t>();

        // Get device name and CAN interface
        device_name_ = config_.value("device_name", "unknown");
        can_interface_ = config_.value("can_interface", "vcan0");

        // Parse objects
        for (auto& [name, obj_json] : config_["objects"].items()) {
            ObjectEntry entry;

            // Parse index as hex string
            std::string index_str = obj_json["index"].get<std::string>();
            entry.index = static_cast<uint16_t>(std::stoul(index_str, nullptr, 16));

            // Parse subindex as uint8_t
            entry.subindex = obj_json["subindex"].get<uint8_t>();

            // Parse datatype from string
            entry.datatype = parse_datatype(obj_json["datatype"].get<std::string>());

            // Parse access type
            entry.access = obj_json["access"].get<std::string>();

            // Optional PDO mapping flag if present
            if (obj_json.contains("pdo_mapping")) {
                entry.pdo_mapping = obj_json["pdo_mapping"].get<std::string>();
            }

            // Optional scaling factor and unit, with defaults
            entry.scaling_factor = obj_json.value("scaling_factor", 1.0);
            entry.unit = obj_json.value("unit", "");

            // Store entry in the objects map
            objects_[name] = entry;
        }
    }

    ObjectDictionary::DataType ObjectDictionary::parse_datatype(const std::string& type_str) {
        if (type_str == "uint8_t") return DataType::UINT8;
        if (type_str == "int8_t") return DataType::INT8;
        if (type_str == "uint16_t") return DataType::UINT16;
        if (type_str == "int16_t") return DataType::INT16;
        if (type_str == "uint32_t") return DataType::UINT32;
        if (type_str == "int32_t") return DataType::INT32;
        throw std::runtime_error("Unknown datatype: " + type_str);
    }

    const ObjectDictionary::ObjectEntry& ObjectDictionary::get_object(
        const std::string& name) const {
        auto it = objects_.find(name);
        if (it == objects_.end()) {
            throw std::runtime_error("Object not found: " + name);
        }
        return it->second;
    }

    size_t ObjectDictionary::ObjectEntry::size_bytes() const {
        switch (datatype) {
        case DataType::UINT8:
        case DataType::INT8:
            return 1;
        case DataType::UINT16:
        case DataType::INT16:
            return 2;
        case DataType::UINT32:
        case DataType::INT32:
            return 4;
        case DataType::UINT64:
        case DataType::INT64:
            return 8;
        default:
            return 0;
        }
    }

    template<typename T>
    std::vector<uint8_t> ObjectDictionary::to_raw(T value) const {
        std::vector<uint8_t> bytes(sizeof(T));
        // Little-endian conversion
        for (size_t i = 0; i < sizeof(T); ++i) {
            bytes[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
        }
        return bytes;
    }

    // Explicit template instantiations
    template std::vector<uint8_t> ObjectDictionary::to_raw<uint8_t>(uint8_t) const;
    template std::vector<uint8_t> ObjectDictionary::to_raw<int8_t>(int8_t) const;
    template std::vector<uint8_t> ObjectDictionary::to_raw<uint16_t>(uint16_t) const;
    template std::vector<uint8_t> ObjectDictionary::to_raw<int16_t>(int16_t) const;
    template std::vector<uint8_t> ObjectDictionary::to_raw<uint32_t>(uint32_t) const;
    template std::vector<uint8_t> ObjectDictionary::to_raw<int32_t>(int32_t) const;

    template<typename T>
    T ObjectDictionary::from_raw(const std::vector<uint8_t>& raw_data) const {
        if (raw_data.size() < sizeof(T)) {
            throw std::runtime_error("Insufficient data for type conversion");
        }

        T value = 0;
        for (size_t i = 0; i < sizeof(T); ++i) {
            value |= static_cast<T>(raw_data[i]) << (i * 8);
        }
        return value;
    }

    // Explicit template instantiations for from_raw
    template uint8_t ObjectDictionary::from_raw<uint8_t>(const std::vector<uint8_t>&) const;
    template int8_t ObjectDictionary::from_raw<int8_t>(const std::vector<uint8_t>&) const;
    template uint16_t ObjectDictionary::from_raw<uint16_t>(const std::vector<uint8_t>&) const;
    template int16_t ObjectDictionary::from_raw<int16_t>(const std::vector<uint8_t>&) const;
    template uint32_t ObjectDictionary::from_raw<uint32_t>(const std::vector<uint8_t>&) const;
    template int32_t ObjectDictionary::from_raw<int32_t>(const std::vector<uint8_t>&) const;

    bool ObjectDictionary::has_object(const std::string& name) const {
        return objects_.find(name) != objects_.end();
    }

    std::vector<std::string> ObjectDictionary::get_pdo_objects(const std::string& pdo_name) const {
        std::vector<std::string> result;
        for (const auto& [name, entry] : objects_) {
            if (entry.pdo_mapping == pdo_name) {
                result.push_back(name);
            }
        }
        return result;
    }

    double ObjectDictionary::get_motor_param(const std::string& param_name) const {
        if (!config_.contains("motor_parameters")) {
            throw std::runtime_error("No motor_parameters section in config");
        }

        auto& params = config_["motor_parameters"];
        if (!params.contains(param_name)) {
            throw std::runtime_error("Motor parameter not found: " + param_name);
        }

        return params[param_name].get<double>();
    }

} // namespace canopen
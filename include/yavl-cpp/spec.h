#pragma once

#include "yavl-cpp/convert.h"

#include <any>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "yaml-cpp/yaml.h"

struct SpecType {
  std::optional<std::vector<std::string>> ExtraIncludes;
  std::optional<std::tuple<std::string, std::string>> CustomCodeGenerator;
  tsl::ordered_map<std::string, YAML::Node> Types;
};

inline void operator>>(const YAML::Node &input, SpecType &output) {
  const std::unordered_map<std::string, bool> keys = {
    {"ExtraIncludes", false},
    {"CustomCodeGenerator", false},
    {"Types", true}
  };
  for (const auto &[key, is_required] : keys) {
    if (is_required && !input[key]) {
      throw YAVL::MissingKeyException("SpecType", key);
    }
  }
  for (const auto &it : input) {
    const std::string key = it.first.as<std::string>();
    if (!keys.contains(key)) {
      throw YAVL::SuperfluousKeyException("SpecType", key);
    }
  }
  input["ExtraIncludes"] >> output.ExtraIncludes;
  input["CustomCodeGenerator"] >> output.CustomCodeGenerator;
  input["Types"] >> output.Types;
}

inline YAML::Emitter& operator<<(YAML::Emitter &output, const SpecType &input) {
  output << YAML::BeginMap;
  output << YAML::Key << "ExtraIncludes";
  output << YAML::Value << input.ExtraIncludes;
  output << YAML::Key << "CustomCodeGenerator";
  output << YAML::Value << input.CustomCodeGenerator;
  output << YAML::Key << "Types";
  output << YAML::Value << input.Types;
  output << YAML::EndMap;
  return output;
}

inline std::vector<std::string> get_types() {
  return {
    "SpecType"
  };
}

inline std::tuple<bool, std::optional<std::string>> validate_simple(const YAML::Node &node, const std::string type_name) {
  if (type_name == "SpecType") {
    return validate<SpecType>(node);
  }
  return std::make_tuple(false, std::nullopt);
}


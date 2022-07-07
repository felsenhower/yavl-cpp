#pragma once

#include <yaml-cpp/yaml.h>
#include "yavl-cpp/convert.h"

#include <any>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "yaml-cpp/yaml.h"

enum TargetType {
  C,
  CPP
};

inline void operator>>(const YAML::Node &input, TargetType &output) {
  std::string tmp;
  input >> tmp;
  if (tmp == "C") {
    output = C;
  } else if (tmp == "CPP") {
    output = CPP;
  } else {
    throw YAVL::BadConversionException(input, "TargetType");
  }
}
inline YAML::Emitter& operator<<(YAML::Emitter &output, const TargetType &input) {
  if (input == C) {
    output << "C";
  } else if (input == CPP) {
    output << "CPP";
  }
  return output;
}

struct SpecType {
  std::optional<std::vector<std::string>> ExtraIncludes;
  std::optional<std::string> Namespace;
  std::optional<TargetType> Target;
  tsl::ordered_map<std::string, YAML::Node> Types;
};

inline void operator>>(const YAML::Node &input, SpecType &output) {
  const std::unordered_map<std::string, bool> keys = {
    {"ExtraIncludes", false},
    {"Namespace", false},
    {"Target", false},
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
  input["Namespace"] >> output.Namespace;
  input["Target"] >> output.Target;
  input["Types"] >> output.Types;
}

inline YAML::Emitter& operator<<(YAML::Emitter &output, const SpecType &input) {
  output << YAML::BeginMap;
  output << YAML::Key << "ExtraIncludes";
  output << YAML::Value << input.ExtraIncludes;
  output << YAML::Key << "Namespace";
  output << YAML::Value << input.Namespace;
  output << YAML::Key << "Target";
  output << YAML::Value << input.Target;
  output << YAML::Key << "Types";
  output << YAML::Value << input.Types;
  output << YAML::EndMap;
  return output;
}

inline std::vector<std::string> get_types() {
  return {
    "TargetType",
    "SpecType"
  };
}

inline std::tuple<bool, std::optional<std::string>> validate_simple(const YAML::Node &node, const std::string type_name) {
  if (type_name == "TargetType") {
    return validate<TargetType>(node);
  } else if (type_name == "SpecType") {
    return validate<SpecType>(node);
  }
  return std::make_tuple(false, std::nullopt);
}


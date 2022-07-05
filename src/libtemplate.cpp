#include "spec.h"

extern "C" {

std::tuple<bool, std::optional<std::string>> validate_simple(const YAML::Node &node, const std::string type_name) {
  return validate(node, type_name);
}
}

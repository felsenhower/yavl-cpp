#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <yaml-cpp/yaml.h>

template<typename T>
inline void operator>>(const YAML::Node &node, T &obj) {
  obj = node.as<T>();
}

template<typename T>
inline void operator>>(const YAML::Node &node, std::vector<T> &obj) {
  for (const auto &it : node) {
    T tmp;
    (YAML::Node) it >> tmp;
    obj.push_back(tmp);
  }
}

template<typename KT, typename VT>
inline void operator>>(const YAML::Node &node, std::map<KT, VT> &obj) {
  for (const auto &it : node) {
    KT key;
    VT val;
    it.first >> key;
    it.second >> val;
    obj[key] = val;
  }
}

template<typename KT, typename VT>
inline void operator>>(const YAML::Node &node, std::unordered_map<KT, VT> &obj) {
  std::map<KT, VT> ordered_map;
  node >> ordered_map;
  std::copy(ordered_map.begin(), ordered_map.end(), std::back_inserter(obj));
}

template<typename T>
inline std::tuple<bool, std::optional<std::string>> validate(const YAML::Node &node) {
  try {
    T tmp;
    node >> tmp;
  } catch (const YAML::Exception &e) { return std::make_tuple(false, e.what()); }
  return std::make_tuple(true, std::nullopt);
}

namespace YAVL {

using validation_result = std::tuple<bool, std::optional<std::string>>;
using validation_function = std::function<validation_result(const YAML::Node &node, const std::string type_name)>;
using type_list = std::vector<std::string>;
using get_types_function = std::function<type_list()>;

struct SymbolTable {
    validation_function validate_simple;
    get_types_function get_types;
};

class BadConversionException : public YAML::RepresentationException {
  public:
    explicit BadConversionException(const YAML::Node &node, const std::string &type_name)
        : YAML::RepresentationException(YAML::Mark::null_mark(),
            std::string("Bad conversion from value \"") + node.as<std::string>() + "\" to type \"" + type_name + "\"") {
    }
};

class MissingKeyException : public YAML::RepresentationException {
  public:
    explicit MissingKeyException(const std::string &type_name, const std::string &key_name)
        : YAML::RepresentationException(YAML::Mark::null_mark(),
            std::string("Missing key \"") + key_name + "\" during conversion to type \"" + type_name + "\"") {}
};

class SuperfluousKeyException : public YAML::RepresentationException {
  public:
    explicit SuperfluousKeyException(const std::string &type_name, const std::string &key_name)
        : YAML::RepresentationException(YAML::Mark::null_mark(),
            std::string("Superfluous key \"") + key_name + "\" during conversion to type \"" + type_name + "\"") {}
};

} // namespace YAVL

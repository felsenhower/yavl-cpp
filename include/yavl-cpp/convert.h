#pragma once

#include <optional>
#include <tuple>
#include <vector>
#include <map>
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

template<typename T>
inline std::tuple<bool, std::optional<std::string>> validate(const YAML::Node &node) {
  try {
    T tmp;
    node >> tmp;
  } catch (const YAML::Exception &e) { return std::make_tuple(false, e.what()); }
  return std::make_tuple(true, std::nullopt);
}

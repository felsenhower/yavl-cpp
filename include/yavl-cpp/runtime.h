#pragma once

#include <algorithm>
#include <any>
#include <array>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <yaml-cpp/yaml.h>

#include "tsl/ordered_map.h"
#include "tsl/ordered_set.h"

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

class DuplicateSetItemException : public YAML::RepresentationException {
  public:
    explicit DuplicateSetItemException()
        : YAML::RepresentationException(YAML::Mark::null_mark(), std::string("Duplicate key in set")) {}
};

class DuplicateMapItemException : public YAML::RepresentationException {
  public:
    explicit DuplicateMapItemException()
        : YAML::RepresentationException(YAML::Mark::null_mark(), std::string("Duplicate key in map")) {}
};

class InvalidSequenceLengthException : public YAML::RepresentationException {
  public:
    explicit InvalidSequenceLengthException(std::size_t expected, std::size_t got)
        : YAML::RepresentationException(YAML::Mark::null_mark(),
            std::string("Invalid sequence length \"" + std::to_string(got) + "\", expected \""
                + std::to_string(expected) + "\"")) {}
};

} // namespace YAVL

template<typename T>
inline void operator>>(const YAML::Node &node, T &obj);

template<typename T>
inline void operator>>(const YAML::Node &node, std::vector<T> &obj);

template<typename T, std::size_t N>
inline void operator>>(const YAML::Node &node, std::array<T, N> &obj);

template<typename T, std::size_t N>
inline void operator>>(const YAML::Node &node, T (&obj)[N]);

template<typename T>
inline void operator>>(const YAML::Node &node, tsl::ordered_set<T> &obj);

template<typename T>
inline void operator>>(const YAML::Node &node, std::set<T> &obj);

template<typename T>
inline void operator>>(const YAML::Node &node, std::unordered_set<T> &obj);

template<typename KT, typename VT>
inline void operator>>(const YAML::Node &node, tsl::ordered_map<KT, VT> &obj);

template<typename KT, typename VT>
inline void operator>>(const YAML::Node &node, std::map<KT, VT> &obj);

template<typename KT, typename VT>
inline void operator>>(const YAML::Node &node, std::unordered_map<KT, VT> &obj);

template<class... Types>
inline void operator>>(const YAML::Node &node, std::tuple<Types...> &obj);

template<typename T>
inline void operator>>(const YAML::Node &node, std::optional<T> &obj);

template<typename T>
inline std::tuple<bool, std::optional<std::string>> validate(const YAML::Node &node) {
  try {
    T tmp;
    node >> tmp;
  } catch (const YAML::Exception &e) { return std::make_tuple(false, e.what()); }
  return std::make_tuple(true, std::nullopt);
}

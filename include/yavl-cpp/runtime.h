#pragma once

#include <algorithm>
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

template<typename T>
inline void operator>>(const YAML::Node &node, std::set<T> &obj) {
  std::vector<T> vec;
  node >> vec;
  std::copy(vec.begin(), vec.end(), std::inserter(obj, obj.begin()));
  if (obj.size() != vec.size()) {
    throw YAVL::DuplicateSetItemException();
  }
}

template<typename T>
inline void operator>>(const YAML::Node &node, std::unordered_set<T> &obj) {
  std::set<T> ordered_set;
  node >> ordered_set;
  std::copy(ordered_set.begin(), ordered_set.end(), std::inserter(obj, obj.begin()));
}

template<typename T>
inline YAML::Emitter &operator<<(YAML::Emitter &output, const std::unordered_set<T> &input) {
  std::vector<T> vec;
  std::copy(input.begin(), input.end(), std::back_inserter(vec));
  return output << vec;
}

template<typename T, std::size_t N>
inline void operator>>(const YAML::Node &node, std::array<T, N> &obj) {
  std::vector<T> vec;
  node >> vec;
  if (vec.size() != N) {
    throw YAVL::InvalidSequenceLengthException(N, vec.size());
  }
  std::copy(vec.begin(), vec.end(), obj.begin());
}

template<typename T, std::size_t N>
inline YAML::Emitter &operator<<(YAML::Emitter &output, const std::array<T, N> &input) {
  std::vector<T> vec;
  std::copy(input.begin(), input.end(), std::back_inserter(vec));
  return output << vec;
}

template<typename T, std::size_t N>
inline YAML::Emitter &operator<<(YAML::Emitter &output, const T (&input)[N]) {
  std::vector<T> vec;
  std::copy(std::begin(input), std::end(input), std::back_inserter(vec));
  return output << vec;
}

template<typename T, std::size_t N>
inline void operator>>(const YAML::Node &node, T (&obj)[N]) {
  std::array<T, N> stdarr;
  node >> stdarr;
  std::copy(stdarr.begin(), stdarr.end(), std::begin(obj));
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
  if (obj.size() != node.size()) {
    throw YAVL::DuplicateMapItemException();
  }
}

template<typename KT, typename VT>
inline void operator>>(const YAML::Node &node, std::unordered_map<KT, VT> &obj) {
  std::map<KT, VT> ordered_map;
  node >> ordered_map;
  std::copy(ordered_map.begin(), ordered_map.end(), std::inserter(obj, obj.end()));
}

template<typename KT, typename VT>
inline YAML::Emitter &operator<<(YAML::Emitter &output, const std::unordered_map<KT, VT> &input) {
  std::map<KT, VT> ordered_map;
  std::copy(input.begin(), input.end(), std::inserter(ordered_map, ordered_map.end()));
  return output << ordered_map;
}

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type tuple_unpack(
    std::tuple<Tp...> &t, std::vector<YAML::Node> vec) {
  // Nothing
}

template<std::size_t I = 0, typename... Tp>
    inline typename std::enable_if
    < I<sizeof...(Tp), void>::type tuple_unpack(std::tuple<Tp...> &t, std::vector<YAML::Node> vec) {
  using T = std::tuple_element_t<I, std::tuple<Tp...>>;
  T tmp;
  vec[0] >> tmp;
  std::get<I>(t) = tmp;
  vec.erase(vec.begin());
  tuple_unpack<I + 1, Tp...>(t, vec);
}

template<class... Types>
inline void operator>>(const YAML::Node &node, std::tuple<Types...> &obj) {
  const std::vector<YAML::Node> vec(node.begin(), node.end());
  tuple_unpack(obj, vec);
}

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type tuple_pack(std::tuple<Tp...> &t, YAML::Emitter &output) {
}

template<std::size_t I = 0, typename... Tp>
    inline typename std::enable_if
    < I<sizeof...(Tp), void>::type tuple_pack(std::tuple<Tp...> &t, YAML::Emitter &output) {
  output << std::get<I>(t);
  tuple_pack<I + 1, Tp...>(t, output);
}

template<class... Types>
inline YAML::Emitter &operator<<(YAML::Emitter &output, const std::tuple<Types...> &input) {
  output << YAML::BeginSeq;
  std::tuple<Types...> tmp = input;
  tuple_pack(tmp, output);
  output << YAML::EndSeq;
  return output;
}

template<typename T>
inline std::tuple<bool, std::optional<std::string>> validate(const YAML::Node &node) {
  try {
    T tmp;
    node >> tmp;
  } catch (const YAML::Exception &e) { return std::make_tuple(false, e.what()); }
  return std::make_tuple(true, std::nullopt);
}

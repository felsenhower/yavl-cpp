#pragma once

#include "yavl-cpp/runtime.h"

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

template<typename T>
inline void operator>>(const YAML::Node &node, tsl::ordered_set<T> &obj) {
  std::vector<T> vec;
  node >> vec;
  std::copy(vec.begin(), vec.end(), std::inserter(obj, obj.begin()));
  if (obj.size() != vec.size()) {
    throw YAVL::DuplicateSetItemException();
  }
}

template<typename T>
inline YAML::Emitter &operator<<(YAML::Emitter &output, const tsl::ordered_set<T> &input) {
  std::vector<T> vec;
  std::copy(input.begin(), input.end(), std::back_inserter(vec));
  return output << vec;
}

template<typename T>
inline void operator>>(const YAML::Node &node, std::set<T> &obj) {
  tsl::ordered_set<T> tmp;
  node >> tmp;
  std::copy(tmp.begin(), tmp.end(), std::inserter(obj, obj.begin()));
}

template<typename T>
inline void operator>>(const YAML::Node &node, std::unordered_set<T> &obj) {
  tsl::ordered_set<T> tmp;
  node >> tmp;
  std::copy(tmp.begin(), tmp.end(), std::inserter(obj, obj.begin()));
}

template<typename T>
inline YAML::Emitter &operator<<(YAML::Emitter &output, const std::unordered_set<T> &input) {
  std::vector<T> vec;
  std::copy(input.begin(), input.end(), std::back_inserter(vec));
  return output << vec;
}

template<typename KT, typename VT>
inline void operator>>(const YAML::Node &node, tsl::ordered_map<KT, VT> &obj) {
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
inline YAML::Emitter &operator<<(YAML::Emitter &output, const tsl::ordered_map<KT, VT> &input) {
  output << YAML::BeginMap;
  for (const auto &[key, value] : input) {
    output << YAML::Key << key << YAML::Value << value;
  }
  output << YAML::EndMap;
  return output;
}

template<typename KT, typename VT>
inline void operator>>(const YAML::Node &node, std::map<KT, VT> &obj) {
  tsl::ordered_map<KT, VT> tmp;
  node >> tmp;
  std::copy(tmp.begin(), tmp.end(), std::inserter(obj, obj.end()));
}

template<typename KT, typename VT>
inline void operator>>(const YAML::Node &node, std::unordered_map<KT, VT> &obj) {
  tsl::ordered_map<KT, VT> tmp;
  node >> tmp;
  std::copy(tmp.begin(), tmp.end(), std::inserter(obj, obj.end()));
}

template<typename KT, typename VT>
inline YAML::Emitter &operator<<(YAML::Emitter &output, const std::unordered_map<KT, VT> &input) {
  tsl::ordered_map<KT, VT> tmp;
  std::copy(input.begin(), input.end(), std::inserter(tmp, tmp.end()));
  return output << tmp;
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
inline void operator>>(const YAML::Node &node, std::optional<T> &obj) {
  if (node.IsDefined() && !node.IsNull()) {
    T tmp;
    node >> tmp;
    obj = tmp;
  } else {
    obj = std::nullopt;
  }
}

template<typename T>
inline YAML::Emitter &operator<<(YAML::Emitter &output, const std::optional<T> &input) {
  if (input.has_value()) {
    T tmp = input.value();
    return output << tmp;
  }
  return output;
}

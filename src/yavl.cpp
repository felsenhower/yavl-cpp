#include <assert.h>
#include <stdio.h>
#include <yaml-cpp/yaml.h>

#include "yavl.h"

namespace YAVL {

template<>
std::string ctype2str<unsigned long long>() {
  return "unsigned long long";
}

template<>
std::string ctype2str<std::string>() {
  return "std::string";
}

template<>
std::string ctype2str<long long>() {
  return "long long";
}

template<>
std::string ctype2str<unsigned int>() {
  return "unsigned int";
}

template<>
std::string ctype2str<int>() {
  return "int";
}

const std::string &Validator::type2str(YAML::NodeType::value t) {
  static std::string nonestr = "none";
  static std::string scalarstr = "scalar";
  static std::string liststr = "list";
  static std::string mapstr = "map";
  static std::string undefinedstr = "undefined";

  switch (t) {
    case YAML::NodeType::Null:
      return nonestr;
    case YAML::NodeType::Scalar:
      return scalarstr;
    case YAML::NodeType::Sequence:
      return liststr;
    case YAML::NodeType::Map:
      return mapstr;
    case YAML::NodeType::Undefined:
      return undefinedstr;
  }
  assert(0);
  return nonestr;
}

int Validator::num_keys(const YAML::Node &doc) {
  if (!doc.IsMap()) {
    return 0;
  }
  return doc.size();
}

bool Validator::validate_map(const YAML::Node &mapNode, const YAML::Node &doc) {
  if (!doc.IsMap()) {
    std::string reason = "expected map, but found " + type2str(doc.Type());
    gen_error(Exception(reason, gr_path, doc_path));
    return false;
  }

  bool ok = true;
  for (const auto &it : mapNode) {
    std::string key = it.first.as<std::string>();
    const YAML::Node &valueNode = it.second;
    if (!doc[key]) {
      std::string reason = "key: " + key + " not found.";
      gen_error(Exception(reason, gr_path, doc_path));
      ok = false;
    } else {
      doc_path.push_back(key);
      gr_path.push_back(key);

      ok = validate_doc(valueNode, doc[key]) && ok;

      gr_path.pop_back();
      doc_path.pop_back();
    }
  }
  return ok;
}

bool Validator::validate_leaf(const YAML::Node &gr, const YAML::Node &doc) {
  assert(gr.IsSequence());
  const YAML::Node &typespec_map = gr[0];
  assert(typespec_map.size() == 1);

  std::string type = typespec_map.begin()->first.as<std::string>();
  const YAML::Node &type_specifics = typespec_map.begin()->second;

  bool ok = true;
  if (type == "std::string") {
    attempt_to_convert<std::string>(doc, ok);
  } else if (type == "uint64") {
    attempt_to_convert<unsigned long long>(doc, ok);
  } else if (type == "int64") {
    attempt_to_convert<long long>(doc, ok);
  } else if (type == "int") {
    attempt_to_convert<int>(doc, ok);
  } else if (type == "uint") {
    attempt_to_convert<unsigned int>(doc, ok);
  } else if (type == "bool") {
    attempt_to_convert<bool>(doc, ok);
  } else if (type == "enum") {
    ok = false;
    for (const auto &it : type_specifics) {
      if (it == doc) {
        ok = true;
        break;
      }
    }
    if (!ok) {
      std::string reason = "enum std::string '" + doc.as<std::string>() + "' is not allowed.";
      gen_error(Exception(reason, gr_path, doc_path));
    }
  }
  return ok;
}

bool Validator::validate_list(const YAML::Node &gr, const YAML::Node &doc) {
  if (!doc.IsSequence()) {
    std::string reason = "expected list, but found " + type2str(doc.Type());
    gen_error(Exception(reason, gr_path, doc_path));
    return false;
  }

  bool ok = true;
  int n = 0;
  char buf[128];

  for (const auto &it : doc) {
    snprintf(buf, sizeof(buf), "[%d]", n);
    doc_path.push_back(buf);
    ok = validate_doc(gr, it) && ok;
    doc_path.pop_back();
    n++;
  }
  return ok;
}

bool Validator::validate_doc(const YAML::Node &gr, const YAML::Node &doc) {
  bool ok = true;

  if (gr["map"]) {
    gr_path.push_back("map");
    ok = validate_map(gr["map"], doc) && ok;
    gr_path.pop_back();
  } else if (gr["list"]) {
    gr_path.push_back("list");
    ok = validate_list(gr["list"], doc) && ok;
    gr_path.pop_back();
  } else {
    ok = validate_leaf(gr, doc) && ok;
  }

  return ok;
}

} // namespace YAVL

std::ostream &operator<<(std::ostream &os, const YAVL::Path &path) {
  bool first = true;
  for (const auto &it : path) {
    // no dot before list indexes and before first element
    if (!first && it[0] != '[') {
      os << '.';
    }
    first = false;
    os << it;
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const YAVL::Exception &v) {
  os << "REASON: " << v.why << std::endl;
  os << "  doc path: " << v.doc_path << std::endl;
  os << "  treespec path: " << v.gr_path << std::endl;
  os << std::endl;
  return os;
}

std::ostream &operator<<(std::ostream &os, const YAVL::Errors &v) {
  for (const auto &it : v) {
    os << it;
  }
  return os;
}

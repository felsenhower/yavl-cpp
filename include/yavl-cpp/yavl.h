#pragma once

#include <ostream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace YAVL {

using Path = std::vector<std::string>;

// really sucks that I have to do this sort of crap since I can't
// pass a type as an argument to a function.
template<typename T>
std::string ctype2str() {
  return "FAIL";
}

class Exception {
  public:
    std::string why;
    Path gr_path;
    Path doc_path;
    Exception(const std::string _why, const Path &_gr_path, const Path &_doc_path)
        : why(_why),
          gr_path(_gr_path),
          doc_path(_doc_path){};
};

using Errors = std::vector<Exception>;

class Validator {
    const YAML::Node &gr;
    const YAML::Node &doc;
    Path gr_path;
    Path doc_path;
    Errors errors;

    int num_keys(const YAML::Node &doc);
    static const std::string &type2str(const YAML::NodeType::value t);
    bool validate_map(const YAML::Node &mapNode, const YAML::Node &doc);
    bool validate_leaf(const YAML::Node &gr, const YAML::Node &doc);
    bool validate_list(const YAML::Node &gr, const YAML::Node &doc);
    bool validate_doc(const YAML::Node &gr, const YAML::Node &doc);

    void gen_error(const Exception &err) {
      errors.push_back(err);
    }

    template<typename T>
    void attempt_to_convert(const YAML::Node &scalar_node, bool &ok) {
      try {
        scalar_node.as<T>();
        ok = true;
      } catch (const YAML::Exception &e) {
        std::string reason =
            "unable to convert '" + type2str(scalar_node.Type()) + "' to '" + YAVL::ctype2str<T>() + "'.";
        gen_error(Exception(reason, gr_path, doc_path));
        ok = false;
      }
    }

  public:
    Validator(const YAML::Node &_gr, const YAML::Node &_doc) : gr(_gr), doc(_doc){};
    bool validate() {
      return validate_doc(gr, doc);
    }
    const Errors &get_errors() {
      return errors;
    }
};
} // namespace YAVL

std::ostream &operator<<(std::ostream &os, const YAVL::Path &path);
std::ostream &operator<<(std::ostream &os, const YAVL::Exception &v);
std::ostream &operator<<(std::ostream &os, const YAVL::Errors &v);

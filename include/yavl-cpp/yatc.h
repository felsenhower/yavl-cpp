#pragma once

#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace YAVL {
std::string to_lower_copy(const std::string &s);

// intentionally named 'kind' to avoid confusion with 'type' :)
enum KindOfDataNodeDefinition {
  BUILTIN,
  VECTOR,
  STRUCT,
  ENUM
};

struct EnumDefinition {
    std::string name;
    std::vector<std::string> enum_values;
};

struct DataNodeDefinition {
    KindOfDataNodeDefinition kind_of_node; // what kind of definition is this
    std::string name; // identifier to which data in this node will be bound
    std::string type; // C++-compatible type
    EnumDefinition enum_def; // if node is for an ENUM
    DataNodeDefinition *listelem_def; // type of list elements if node is for VECTOR
    std::vector<DataNodeDefinition *> elems; // types of members if node is for STRUCT
};

class DataBinderGen {
  public:
    static void emit_header(const YAML::Node &_gr, std::string _topname, std::ostream &os);
    
  protected:
    const YAML::Node &gr; // tree for grammar
    DataNodeDefinition root_data_defn;
    std::string topname; // name of top-level data struct.
    std::ostream &os;
    
    DataBinderGen(const YAML::Node &_gr, std::string _topname, std::ostream &os) : gr(_gr), topname(_topname), os(os) {
      root_data_defn = make_types(gr, topname);
    };

    DataNodeDefinition make_types(const YAML::Node &doc, std::string name);
    DataNodeDefinition make_list_type(const YAML::Node &gr, std::string name);
    DataNodeDefinition make_map_type(const YAML::Node &mapNode, std::string name);
    DataNodeDefinition make_scalar_type(const YAML::Node &gr, std::string name);
    void emit_header();
    void emit_includes();
    void emit_declarations(const DataNodeDefinition &elem);
    void emit_dumper(const DataNodeDefinition &elem);
    void emit_reader(const DataNodeDefinition &elem);
    void emit_enum_declaration(const DataNodeDefinition &elem);
    void emit_enum_reader(const DataNodeDefinition &elem);
    void emit_enum_dumper(const DataNodeDefinition &elem);
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

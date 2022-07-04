#include <algorithm>
#include <cassert>
#include <yaml-cpp/yaml.h>

#include "yavl-cpp/yatc.h"

namespace YAVL {

std::string to_lower_copy(const std::string &s) {
  std::string result = s;
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c){ return std::tolower(c); });
  return result;
}

DataNodeDefinition DataBinderGen::make_map_type(const YAML::Node &mapNode, std::string name) {
  DataNodeDefinition rsl;
  rsl.kind_of_node = STRUCT;
  rsl.type = to_lower_copy(name);
  rsl.type[0] = toupper(rsl.type[0]);
  rsl.name = name;

  for (const auto &it : mapNode) {
    std::string key = it.first.as<std::string>();
    const YAML::Node &valueNode = it.second;
    DataNodeDefinition subrsl = make_types(valueNode, key);
    DataNodeDefinition *e = new DataNodeDefinition(subrsl);
    rsl.elems.push_back(e);
  }
  return rsl;
}

DataNodeDefinition DataBinderGen::make_scalar_type(const YAML::Node &doc, std::string name) {
  assert(doc.IsSequence());

  const YAML::Node &typespec_map = doc[0];
  //assert( num_keys(typespec_map) == 1);

  std::string type = typespec_map.begin()->first.as<std::string>();
  const YAML::Node &type_specifics = typespec_map.begin()->second;

  DataNodeDefinition elem;

  elem.name = name;

  if (type == "string") {
    elem.type = "std::string";
    elem.kind_of_node = BUILTIN;
  } else if (type == "uint64_t") {
    elem.type = "uint64_t";
    elem.kind_of_node = BUILTIN;
  } else if (type == "int64_t") {
    elem.type = "int64_t";
    elem.kind_of_node = BUILTIN;
  } else if (type == "int") {
    elem.type = "int";
    elem.kind_of_node = BUILTIN;
  } else if (type == "uint") {
    elem.type = "unsigned int";
    elem.kind_of_node = BUILTIN;
  } else if (type == "enum") {
    elem.enum_def.name = to_lower_copy(elem.name);
    elem.enum_def.name[0] = toupper(elem.enum_def.name[0]);
    std::transform(type_specifics.begin(), type_specifics.end(), std::back_inserter(elem.enum_def.enum_values),
        [](const auto &it) {return it.template as<std::string>();});
    elem.kind_of_node = ENUM;
    elem.type = elem.enum_def.name;
  }
  return elem;
}

DataNodeDefinition DataBinderGen::make_list_type(const YAML::Node &doc, std::string name) {
  DataNodeDefinition elem;
  elem.name = name;
  elem.kind_of_node = VECTOR;

  //assert(num_keys(doc) == 1);

  std::string vec_kind_of_node = to_lower_copy(name);
  vec_kind_of_node[0] = toupper(vec_kind_of_node[0]);
  if (*(vec_kind_of_node.end() - 1) == 's') {
    vec_kind_of_node.erase(vec_kind_of_node.end() - 1);
  }
  DataNodeDefinition subrsl = make_types(doc, vec_kind_of_node);
  DataNodeDefinition *e = new DataNodeDefinition(subrsl);
  elem.listelem_def = e;

  elem.type = std::string("std::vector<") + elem.listelem_def->type + std::string(" >");

  return elem;
}

DataNodeDefinition DataBinderGen::make_types(const YAML::Node &doc, std::string name) {
  if (doc["map"]) {
    return make_map_type(doc["map"], name);
  } else if (doc["list"]) {
    return make_list_type(doc["list"], name);
  }
  return make_scalar_type(doc, name);
}

void DataBinderGen::emit_enum_declaration(const DataNodeDefinition &elem) {
  os << "enum " << elem.enum_def.name << " { ";
  std::vector<std::string>::const_iterator i = elem.enum_def.enum_values.begin();
  for (; i != elem.enum_def.enum_values.end(); ++i) {
    if (i != elem.enum_def.enum_values.begin()) {
      os << ", ";
    }
    os << *i;
  }
  os << " };" << std::endl;
}

void DataBinderGen::emit_declarations(const DataNodeDefinition &elem) {
  switch (elem.kind_of_node) {
    case ENUM:
      emit_enum_declaration(elem);
      break;

    case VECTOR:
      if (elem.listelem_def->kind_of_node != BUILTIN) {
        emit_declarations(*elem.listelem_def);
      }
      break;

    case BUILTIN:
      break;

    case STRUCT:
      std::vector<DataNodeDefinition *>::const_iterator i = elem.elems.begin();
      // pass one: emit what I depend on
      for (; i != elem.elems.end(); ++i) {
        if (((*i)->kind_of_node == ENUM)) {
          emit_enum_declaration(**i);
        } else if ((*i)->kind_of_node == STRUCT) {
          emit_declarations(**i);
        } else if ((*i)->kind_of_node == VECTOR) {
          if ((*i)->listelem_def->kind_of_node != BUILTIN) {
            emit_declarations(*((*i)->listelem_def));
          }
        }
      }
      // pass two: emit myself
      os << "struct " << elem.type << " {" << std::endl;
      i = elem.elems.begin();
      for (; i != elem.elems.end(); ++i) {
        const DataNodeDefinition &e = **i;
        os << "  " << e.type << " " << e.name << ";" << std::endl;
      }
      os << "};" << std::endl;
      break;
  }
}

void DataBinderGen::emit_includes() {
  os << "#pragma once" << std::endl;
  os << "#include <yaml-cpp/yaml.h>" << std::endl;
  os << "#include \"yavl-cpp/yatc.h\"" << std::endl;
}

void DataBinderGen::emit_enum_reader(const DataNodeDefinition &elem) {
  os << "inline void operator >>(const YAML::Node& node, " << elem.type << " &obj) {" << std::endl;
  os << "  std::string tmp; node >> tmp;" << std::endl;
  std::vector<std::string>::const_iterator i = elem.enum_def.enum_values.begin();
  for (; i != elem.enum_def.enum_values.end(); ++i) {
    os << "  if (tmp == \"" << *i << "\") obj = " << *i << ';' << std::endl;
  }
  os << "}" << std::endl;
}

void DataBinderGen::emit_reader(const DataNodeDefinition &elem) {
  switch (elem.kind_of_node) {
    case ENUM:
      emit_enum_reader(elem);
      os << "inline void operator >>(const YAML::Node& node, " << elem.type << " &obj) {" << std::endl;
      os << "  node[\"" << elem.name << "\"] >> obj." << elem.name << ";" << std::endl;
      os << "}" << std::endl;
      break;

    case VECTOR:
      if (elem.listelem_def->kind_of_node != BUILTIN) {
        emit_reader(*elem.listelem_def);
      }
      break;

    case BUILTIN:
      os << "inline void operator >>(const YAML::Node& node, " << elem.type << " &obj) {" << std::endl;
      os << "  node[\"" << elem.name << "\"] >> obj." << elem.name << ";" << std::endl;
      os << "}" << std::endl;
      break;

    case STRUCT:
      std::vector<DataNodeDefinition *>::const_iterator i = elem.elems.begin();
      // pass one: emit what I depend on
      for (; i != elem.elems.end(); ++i) {
        if (((*i)->kind_of_node == ENUM)) {
          emit_enum_reader(**i);
        } else if ((*i)->kind_of_node == STRUCT) {
          emit_reader(**i);
        } else if ((*i)->kind_of_node == VECTOR) {
          if ((*i)->listelem_def->kind_of_node != BUILTIN) {
            emit_reader(*((*i)->listelem_def));
          }
        }
      }
      // pass two: emit myself
      os << "inline void operator >>(const YAML::Node& node, " << elem.type << " &obj) {" << std::endl;
      i = elem.elems.begin();
      for (; i != elem.elems.end(); ++i) {
        const DataNodeDefinition &e = **i;
        os << "  node[\"" << e.name << "\"] >> obj." << e.name << ";" << std::endl;
      }
      os << "}" << std::endl;
      break;
  }
}

void DataBinderGen::emit_enum_dumper(const DataNodeDefinition &elem) {
  os << "inline YAML::Emitter& operator <<(YAML::Emitter& out, const " << elem.type << " &obj) {" << std::endl;
  std::vector<std::string>::const_iterator i = elem.enum_def.enum_values.begin();
  for (; i != elem.enum_def.enum_values.end(); ++i) {
    os << "  if (obj == " << *i << ") out << \"" << *i << "\";" << std::endl;
  }
  os << "  return out;" << std::endl;
  os << "}" << std::endl;
}

void DataBinderGen::emit_dumper(const DataNodeDefinition &elem) {
  switch (elem.kind_of_node) {
    case ENUM:
      emit_enum_dumper(elem);
      os << "inline YAML::Emitter& operator <<(YAML::Emitter& out, const " << elem.type << " &obj) {" << std::endl;
      os << "  out << obj." << elem.name << ";" << std::endl;
      os << "  return out;" << std::endl;
      os << "}" << std::endl;
      break;

    case VECTOR:
      if (elem.listelem_def->kind_of_node != BUILTIN) {
        emit_dumper(*elem.listelem_def);
      }
      break;

    case BUILTIN:
      break;

    case STRUCT:
      std::vector<DataNodeDefinition *>::const_iterator i = elem.elems.begin();
      // pass one: emit what I depend on
      for (; i != elem.elems.end(); ++i) {
        if (((*i)->kind_of_node == ENUM)) {
          emit_enum_dumper(**i);
        } else if ((*i)->kind_of_node == STRUCT) {
          emit_dumper(**i);
        } else if ((*i)->kind_of_node == VECTOR) {
          if ((*i)->listelem_def->kind_of_node != BUILTIN) {
            emit_dumper(*((*i)->listelem_def));
          }
        }
      }
      // pass two: emit myself
      os << "inline YAML::Emitter& operator <<(YAML::Emitter& out, const " << elem.type << " &obj) {" << std::endl;
      os << "  out << YAML::BeginMap;" << std::endl;
      i = elem.elems.begin();
      for (; i != elem.elems.end(); ++i) {
        const DataNodeDefinition &e = **i;
        os << "  out << YAML::Key << \"" << e.name << "\";" << std::endl;
        os << "  out << YAML::Value << obj." << e.name << ";" << std::endl;
      }
      os << "  out << YAML::EndMap;" << std::endl;
      os << "  return out;" << std::endl;
      os << "}" << std::endl;
      break;
  }
}

void DataBinderGen::emit_header() {
  emit_includes();
  emit_declarations(root_data_defn);
  emit_reader(root_data_defn);
  emit_dumper(root_data_defn);
}

void DataBinderGen::emit_header(const YAML::Node &gr, std::string topname, std::ostream &os) {
  DataBinderGen emitter(gr, topname, os);
  emitter.emit_header();
}
  

} // namespace YAVL

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <yaml-cpp/yaml.h>

#include "yatc.h"

using namespace std;
using namespace YAVL;

string YAVL::to_lower_copy(string s) {
  string rsl(s.size(), ' ');
  for (string::size_type i = 0; i < s.size(); ++i) {
    rsl[i] = tolower(s[i]);
  }
  return rsl;
}

DataNodeDefinition DataBinderGen::make_map_type(const YAML::Node &mapNode, string name) {
  DataNodeDefinition rsl;
  rsl.kind_of_node = STRUCT;
  rsl.type = to_lower_copy(name);
  rsl.type[0] = toupper(rsl.type[0]);
  rsl.name = name;

  for (const auto &it : mapNode) {
    string key = it.first.as<string>();
    const YAML::Node &valueNode = it.second;
    DataNodeDefinition subrsl = make_types(valueNode, key);
    DataNodeDefinition *e = new DataNodeDefinition(subrsl);
    rsl.elems.push_back(e);
  }
  return rsl;
}

DataNodeDefinition DataBinderGen::make_scalar_type(const YAML::Node &doc, string name) {
  assert(doc.IsSequence());

  const YAML::Node &typespec_map = doc[0];
  //assert( num_keys(typespec_map) == 1);

  string type = typespec_map.begin()->first.as<string>();
  const YAML::Node &type_specifics = typespec_map.begin()->second;

  DataNodeDefinition elem;

  elem.name = name;

  bool ok = true;
  if (type == "string") {
    elem.type = "std::string";
    elem.kind_of_node = BUILTIN;
  } else if (type == "uint64") {
    elem.type = "unsigned long long";
    elem.kind_of_node = BUILTIN;
  } else if (type == "int64") {
    elem.type = "long long";
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
    for (const auto &it : type_specifics) {
      elem.enum_def.enum_values.push_back(it.as<string>());
    }
    elem.kind_of_node = ENUM;
    elem.type = elem.enum_def.name;
  } else {
    ok = false;
  }
  return elem;
}

DataNodeDefinition DataBinderGen::make_list_type(const YAML::Node &doc, string name) {
  DataNodeDefinition elem;
  elem.name = name;
  elem.kind_of_node = VECTOR;

  //assert(num_keys(doc) == 1);

  string vec_kind_of_node = to_lower_copy(name);
  vec_kind_of_node[0] = toupper(vec_kind_of_node[0]);
  if (*(vec_kind_of_node.end() - 1) == 's') {
    vec_kind_of_node.erase(vec_kind_of_node.end() - 1);
  }
  DataNodeDefinition subrsl = make_types(doc, vec_kind_of_node);
  DataNodeDefinition *e = new DataNodeDefinition(subrsl);
  elem.listelem_def = e;

  elem.type = string("std::vector<") + elem.listelem_def->type + string(" >");

  return elem;
}

DataNodeDefinition DataBinderGen::make_types(const YAML::Node &doc, string name) {
  if (doc["map"]) {
    return make_map_type(doc["map"], name);
  } else if (doc["list"]) {
    return make_list_type(doc["list"], name);
  }
  return make_scalar_type(doc, name);
}

void DataBinderGen::emit_enum_def(const DataNodeDefinition &elem, ostream &os) {
  os << "enum " << elem.enum_def.name << " { ";
  vector<string>::const_iterator i = elem.enum_def.enum_values.begin();
  for (; i != elem.enum_def.enum_values.end(); ++i) {
    if (i != elem.enum_def.enum_values.begin()) {
      os << ", ";
    }
    os << *i;
  }
  os << " };" << endl;
  os << "void operator >>(const YAML::Node& node, " << elem.type << " &obj);" << endl;
  write_put_operator_prolog(os, elem.type, true /* prototype */);
}

bool DataBinderGen::emit_header(const DataNodeDefinition &elem, ostream &os) {
  switch (elem.kind_of_node) {
    case ENUM:
      emit_enum_def(elem, os);
      break;

    case VECTOR:
      if (elem.listelem_def->kind_of_node != BUILTIN) {
        emit_header(*elem.listelem_def, os);
      }
      break;

    case BUILTIN:
      break;

    case STRUCT:
      vector<DataNodeDefinition *>::const_iterator i = elem.elems.begin();
      // pass one: emit what I depend on
      for (; i != elem.elems.end(); ++i) {
        if (((*i)->kind_of_node == ENUM)) {
          emit_enum_def(**i, os);
        } else if ((*i)->kind_of_node == STRUCT) {
          emit_header(**i, os);
        } else if ((*i)->kind_of_node == VECTOR) {
          if ((*i)->listelem_def->kind_of_node != BUILTIN) {
            emit_header(*((*i)->listelem_def), os);
          }
        }
      }
      // pass two: emit myself
      os << "struct " << elem.type << " {" << endl;
      i = elem.elems.begin();
      for (; i != elem.elems.end(); ++i) {
        const DataNodeDefinition &e = **i;
        os << "  " << e.type << " " << e.name << ";" << endl;
      }
      os << "};" << endl;
      os << "void operator >>(const YAML::Node& node, " << elem.type << " &obj);" << endl;
      write_put_operator_prolog(os, elem.type, true /* prototype */);
      break;
  }
  return true;
}

bool DataBinderGen::emit_header(ostream &os) {
  os << "#include <vector>" << endl;
  os << "#include <string>" << endl;
  os << "#include <yaml-cpp/yaml.h>" << endl;
  os << "#include \"yatc.h\"" << endl;
  emit_header(root_data_defn, os);
  return true;
}

void DataBinderGen::emit_enum_reader(const DataNodeDefinition &elem, ostream &os) {
  os << "void operator >>(const YAML::Node& node, " << elem.type << " &obj) {" << endl;
  os << "  string tmp; node >> tmp;" << endl;
  vector<string>::const_iterator i = elem.enum_def.enum_values.begin();
  for (; i != elem.enum_def.enum_values.end(); ++i) {
    os << "  if (tmp == \"" << *i << "\") obj = " << *i << ';' << endl;
  }
  os << "}" << endl;
}

bool DataBinderGen::emit_reader(const DataNodeDefinition &elem, ostream &os) {
  switch (elem.kind_of_node) {
    case ENUM:
      emit_enum_reader(elem, os);
      os << "void operator >>(const YAML::Node& node, " << elem.type << " &obj) {" << endl;
      os << "  node[\"" << elem.name << "\"] >> obj." << elem.name << ";" << endl;
      os << "}" << endl;
      break;

    case VECTOR:
      if (elem.listelem_def->kind_of_node != BUILTIN) {
        emit_reader(*elem.listelem_def, os);
      }
      break;

    case BUILTIN:
      os << "void operator >>(const YAML::Node& node, " << elem.type << " &obj) {" << endl;
      os << "  node[\"" << elem.name << "\"] >> obj." << elem.name << ";" << endl;
      os << "}" << endl;
      break;

    case STRUCT:
      vector<DataNodeDefinition *>::const_iterator i = elem.elems.begin();
      // pass one: emit what I depend on
      for (; i != elem.elems.end(); ++i) {
        if (((*i)->kind_of_node == ENUM)) {
          emit_enum_reader(**i, os);
        } else if ((*i)->kind_of_node == STRUCT) {
          emit_reader(**i, os);
        } else if ((*i)->kind_of_node == VECTOR) {
          if ((*i)->listelem_def->kind_of_node != BUILTIN) {
            emit_reader(*((*i)->listelem_def), os);
          }
        }
      }
      // pass two: emit myself
      os << "void operator >>(const YAML::Node& node, " << elem.type << " &obj) {" << endl;
      i = elem.elems.begin();
      for (; i != elem.elems.end(); ++i) {
        const DataNodeDefinition &e = **i;
        os << "  node[\"" << e.name << "\"] >> obj." << e.name << ";" << endl;
      }
      os << "}" << endl;
      break;
  }
  return true;
}

bool DataBinderGen::emit_reader(ostream &os) {
  os << "#include \"" << to_lower_copy(topname) << ".h\"" << endl;
  os << "using namespace std;" << endl;
  emit_reader(root_data_defn, os);
  return true;
}

void DataBinderGen::emit_enum_dumper(const DataNodeDefinition &elem, ostream &os) {
  write_put_operator_prolog(os, elem.type);
  vector<string>::const_iterator i = elem.enum_def.enum_values.begin();
  for (; i != elem.enum_def.enum_values.end(); ++i) {
    os << "  if (obj == " << *i << ") out << \"" << *i << "\";" << endl;
  }
  write_put_operator_epilog(os);
}

bool DataBinderGen::write_put_operator_prolog(ostream &os, string type, bool prototype) {
  os << "YAML::Emitter& operator <<(YAML::Emitter& out, const " << type << " &obj)";
  if (!prototype) {
    os << " {";
  } else {
    os << ";";
  }
  os << endl;
  return true;
}

bool DataBinderGen::write_put_operator_epilog(ostream &os) {
  os << "  return out;" << endl;
  os << "}" << endl;
  return true;
}

#if 0
bool DataBinderGen::emit_dumper(const DataNodeDefinition &elem, ostream& os)
{
  switch (elem.kind_of_node) {
    case ENUM:
      emit_enum_dumper(elem, os);
      write_put_operator_prolog(os, elem.type);
      os << "  os << \"" << elem.name << ": \" << obj." << elem.name << " << endl;" << endl;
      write_put_operator_epilog(os);
      break;

    case VECTOR:
       if (elem.listelem_def->kind_of_node != BUILTIN) {
         emit_dumper(*elem.listelem_def, os);
       }
      break;

    case BUILTIN:
      break;

    case STRUCT:
      vector<DataNodeDefinition*>::const_iterator i = elem.elems.begin();
      // pass one: emit what I depend on
      for (; i != elem.elems.end(); ++i) {
        if (((*i)->kind_of_node == ENUM)) {
          emit_enum_dumper(**i, os);
        } else if ((*i)->kind_of_node == STRUCT) {
          emit_dumper(**i, os);
        } else if ((*i)->kind_of_node == VECTOR) {
          if ((*i)->listelem_def->kind_of_node != BUILTIN) {
            emit_dumper(*((*i)->listelem_def), os);
          }
        }
      }
      // pass two: emit myself
      write_put_operator_prolog(os, elem.type);
      i = elem.elems.begin();
      for (; i != elem.elems.end(); ++i) {
        const DataNodeDefinition& e = **i;
        os << "  os << \"" << e.name << ": \" << obj." << e.name << " << endl;" << endl;
      }
      write_put_operator_epilog(os);
      break;
  }
  return true;
}
#endif

bool DataBinderGen::emit_dumper(const DataNodeDefinition &elem, ostream &os) {
  switch (elem.kind_of_node) {
    case ENUM:
      emit_enum_dumper(elem, os);
      write_put_operator_prolog(os, elem.type);
      os << "  out << obj." << elem.name << ";" << endl;
      write_put_operator_epilog(os);
      break;

    case VECTOR:
      if (elem.listelem_def->kind_of_node != BUILTIN) {
        emit_dumper(*elem.listelem_def, os);
      }
      break;

    case BUILTIN:
      break;

    case STRUCT:
      vector<DataNodeDefinition *>::const_iterator i = elem.elems.begin();
      // pass one: emit what I depend on
      for (; i != elem.elems.end(); ++i) {
        if (((*i)->kind_of_node == ENUM)) {
          emit_enum_dumper(**i, os);
        } else if ((*i)->kind_of_node == STRUCT) {
          emit_dumper(**i, os);
        } else if ((*i)->kind_of_node == VECTOR) {
          if ((*i)->listelem_def->kind_of_node != BUILTIN) {
            emit_dumper(*((*i)->listelem_def), os);
          }
        }
      }
      // pass two: emit myself
      write_put_operator_prolog(os, elem.type);
      os << "  out << YAML::BeginMap;" << endl;
      i = elem.elems.begin();
      for (; i != elem.elems.end(); ++i) {
        const DataNodeDefinition &e = **i;
        os << "  out << YAML::Key << \"" << e.name << "\";" << endl;
        os << "  out << YAML::Value << obj." << e.name << ";" << endl;
      }
      os << "  out << YAML::EndMap;" << endl;
      write_put_operator_epilog(os);
      break;
  }
  return true;
}

bool DataBinderGen::emit_dumper(ostream &os) {
  emit_dumper(root_data_defn, os);
  return true;
}

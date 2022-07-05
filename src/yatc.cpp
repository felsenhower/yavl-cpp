#include <algorithm>
#include <cassert>
#include <yaml-cpp/yaml.h>

#include "yavl-cpp/yatc.h"

namespace YAVL {

void CodeGenerator::emit_header(const YAML::Node &spec, std::ostream &outstream, const bool emit_declarations,
    const bool emit_readers, const bool emit_writers) {
  CodeGenerator generator(spec, outstream, emit_declarations, emit_readers, emit_writers);
  generator.emit_header();
}

CodeGenerator::CodeGenerator(const YAML::Node &spec, std::ostream &outstream, const bool emit_declarations,
    const bool emit_readers, const bool emit_writers)
    : spec(spec),
      outstream(outstream),
      emit_declarations(emit_declarations),
      emit_readers(emit_readers),
      emit_writers(emit_writers) {}

void CodeGenerator::emit_header() {
  YAML::Node types = spec["Types"];
  assert(types.IsMap());
  emit_includes();
  for (const auto &type_def : types) {
    const std::string type_name = type_def.first.as<std::string>();
    const YAML::Node type_info = type_def.second;
    emit_type(type_name, type_info);
  }
}

void CodeGenerator::emit_includes() {
  outstream << "#pragma once" << std::endl;
  outstream << "#include <yaml-cpp/yaml.h>" << std::endl;
  outstream << "#include \"yavl-cpp/convert.h\"" << std::endl;
}

void CodeGenerator::emit_type(const std::string &type_name, const YAML::Node &type_info) {
  const bool is_map = type_info.IsMap();
  const bool is_enum = type_info.IsSequence();
  assert(is_map != is_enum);
  if (is_map) {
    if (emit_declarations) {
      emit_map_declaration(type_name, type_info);
    }
    if (emit_readers) {
      emit_map_reader(type_name, type_info);
    }
    if (emit_writers) {
      emit_map_writer(type_name, type_info);
    }
  } else {
    if (emit_declarations) {
      emit_enum_declaration(type_name, type_info);
    }
    if (emit_readers) {
      emit_enum_reader(type_name, type_info);
    }
    if (emit_writers) {
      emit_enum_writer(type_name, type_info);
    }
  }
}

void CodeGenerator::emit_map_declaration(const std::string &type_name, const YAML::Node &type_info) {
  outstream << "struct " << type_name << " {\n";
  for (const auto &field : type_info) {
    const std::string field_name = field.first.as<std::string>();
    const std::string field_type = field.second.as<std::string>();
    outstream << "  " << field_type << " " << field_name << ";\n";
  }
  outstream << "};\n";
}

void CodeGenerator::emit_map_reader(const std::string &type_name, const YAML::Node &type_info) {
  outstream << "inline void operator>>(const YAML::Node& input, " << type_name << " &output) {" << std::endl;
  for (const auto &field : type_info) {
    const std::string field_name = field.first.as<std::string>();
    outstream << "  input[\"" << field_name << "\"] >> output." << field_name << ";" << std::endl;
  }
  outstream << "}\n";
}

void CodeGenerator::emit_map_writer(const std::string &type_name, const YAML::Node &type_info) {
  outstream << "inline YAML::Emitter& operator<<(YAML::Emitter& output, const " << type_name << " &input) {"
            << std::endl;
  outstream << "  output << YAML::BeginMap;" << std::endl;
  for (const auto &field : type_info) {
    const std::string field_name = field.first.as<std::string>();
    outstream << "  output << YAML::Key << \"" << field_name << "\";" << std::endl;
    outstream << "  output << YAML::Value << input." << field_name << ";" << std::endl;
  }
  outstream << "  output << YAML::EndMap;" << std::endl;
  outstream << "  return output;\n";
  outstream << "}" << std::endl;
}

void CodeGenerator::emit_enum_declaration(const std::string &type_name, const YAML::Node &type_info) {
  outstream << "enum " << type_name << " {";
  bool first = true;
  for (const auto &choice : type_info) {
    assert(choice.IsScalar());
    if (!first) {
      outstream << ",";
    }
    first = false;
    outstream << "\n";
    outstream << "  " << choice.as<std::string>();
  }
  outstream << "\n};"
            << "\n";
}

void CodeGenerator::emit_enum_reader(const std::string &type_name, const YAML::Node &type_info) {
  outstream << "inline void operator>>(const YAML::Node& input, " << type_name << " &output) {" << std::endl;
  outstream << "  std::string tmp;\n  input >> tmp;" << std::endl;
  bool first = true;
  for (const auto &choice : type_info) {
    if (first) {
      outstream << "  ";
    } else {
      outstream << " else ";
    }
    first = false;
    outstream << "if (tmp == \"" << choice << "\") {\n    output = " << choice << ";\n  }";
  }
  outstream << "\n}" << std::endl;
}

void CodeGenerator::emit_enum_writer(const std::string &type_name, const YAML::Node &type_info) {
  outstream << "inline YAML::Emitter& operator<<(YAML::Emitter& output, const " << type_name << " &input) {"
            << std::endl;
  bool first = true;
  for (const auto &choice : type_info) {
    if (first) {
      outstream << "  ";
    } else {
      outstream << " else ";
    }
    first = false;
    outstream << "if (input == " << choice << ") {\n    output << \"" << choice << "\";\n  }";
  }
  outstream << "  return output;\n";
  outstream << "}\n" << std::endl;
}

} // namespace YAVL

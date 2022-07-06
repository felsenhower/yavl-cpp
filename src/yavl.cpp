#include <algorithm>
#include <cassert>
#include <optional>
#include <yaml-cpp/yaml.h>

#include "yavl-cpp/yavl.h"

namespace YAVL {

void CodeGenerator::emit_header(const YAML::Node &spec, std::ostream &outstream, const bool is_emit_declarations,
    const bool is_emit_readers, const bool is_emit_writers, const bool is_emit_validator) {
  CodeGenerator generator(spec, outstream, is_emit_declarations, is_emit_readers, is_emit_writers, is_emit_validator);
  generator.emit_header();
}

CodeGenerator::CodeGenerator(const YAML::Node &spec, std::ostream &outstream, const bool is_emit_declarations,
    const bool is_emit_readers, const bool is_emit_writers, const bool is_emit_validator)
    : spec(spec),
      outstream(outstream),
      is_emit_declarations(is_emit_declarations),
      is_emit_readers(is_emit_readers),
      is_emit_writers(is_emit_writers),
      is_emit_validator(is_emit_validator) {}

void CodeGenerator::emit_header() {
  YAML::Node types = spec["Types"];
  assert(types.IsMap());
  emit_includes();
  for (const auto &type_def : types) {
    const std::string type_name = type_def.first.as<std::string>();
    const YAML::Node type_info = type_def.second;
    emit_type(type_name, type_info);
  }
  if (is_emit_validator) {
    emit_validator();
  }
}

void CodeGenerator::emit_includes() {
  outstream << "#pragma once" << std::endl << std::endl;
  if (is_emit_readers || is_emit_writers || is_emit_validator) {
    outstream << "#include <yaml-cpp/yaml.h>" << std::endl
              << "#include \"yavl-cpp/runtime.h\"" << std::endl
              << std::endl;
  }
  YAML::Node extra_includes = spec["ExtraIncludes"];
  if (extra_includes.IsSequence()) {
    for (const auto &header : extra_includes) {
      outstream << "#include " << header.as<std::string>() << std::endl;
    }
    outstream << std::endl;
  }
}

void CodeGenerator::emit_type(const std::string &type_name, const YAML::Node &type_info) {
  const bool is_map = type_info.IsMap();
  const bool is_enum = type_info.IsSequence();
  assert(is_map != is_enum);
  if (is_map) {
    if (is_emit_declarations) {
      emit_map_declaration(type_name, type_info);
    }
    if (is_emit_readers) {
      emit_map_reader(type_name, type_info);
    }
    if (is_emit_writers) {
      emit_map_writer(type_name, type_info);
    }
  } else {
    if (is_emit_declarations) {
      emit_enum_declaration(type_name, type_info);
    }
    if (is_emit_readers) {
      emit_enum_reader(type_name, type_info);
    }
    if (is_emit_writers) {
      emit_enum_writer(type_name, type_info);
    }
  }
}

void CodeGenerator::emit_map_declaration(const std::string &type_name, const YAML::Node &type_info) {
  outstream << "struct " << type_name << " {" << std::endl;
  for (const auto &field : type_info) {
    const std::string field_name = field.first.as<std::string>();
    const std::string field_type = field.second.as<std::string>();
    auto bracket_pos = field_type.find("[");
    bool is_c_array_type = (bracket_pos != std::string::npos);
    if (is_c_array_type) {
      const std::string base_type = field_type.substr(0, bracket_pos);
      const std::string arr_length = field_type.substr(bracket_pos);
      outstream << "  " << base_type << " " << field_name << arr_length << ";" << std::endl;
    } else {
      outstream << "  " << field_type << " " << field_name << ";" << std::endl;
    }
  }
  outstream << "};" << std::endl << std::endl;
}

void CodeGenerator::emit_map_reader(const std::string &type_name, const YAML::Node &type_info) {
  outstream << "inline void operator>>(const YAML::Node &input, " << type_name << " &output) {" << std::endl
            << "  const std::set<std::string> keys = {";
  bool first = true;
  for (const auto &field : type_info) {
    if (!first) {
      outstream << ",";
    }
    first = false;
    const std::string field_name = field.first.as<std::string>();
    outstream << std::endl << "    \"" << field_name << "\"";
  }
  outstream << std::endl
            << "  };" << std::endl
            << "  for (const auto &key : keys) {" << std::endl
            << "    if (!input[key]) {" << std::endl
            << "      throw YAVL::MissingKeyException(\"" << type_name << "\", key);" << std::endl
            << "    }" << std::endl
            << "  }" << std::endl
            << "  for (const auto &it : input) {" << std::endl
            << "    const std::string key = it.first.as<std::string>();" << std::endl
            << "    if (!keys.contains(key)) {" << std::endl
            << "      throw YAVL::SuperfluousKeyException(\"" << type_name << "\", key);" << std::endl
            << "    }" << std::endl
            << "  }" << std::endl;
  for (const auto &field : type_info) {
    const std::string field_name = field.first.as<std::string>();
    outstream << "  input[\"" << field_name << "\"] >> output." << field_name << ";" << std::endl;
  }
  outstream << "}" << std::endl << std::endl;
}

void CodeGenerator::emit_map_writer(const std::string &type_name, const YAML::Node &type_info) {
  outstream << "inline YAML::Emitter& operator<<(YAML::Emitter &output, const " << type_name << " &input) {"
            << std::endl
            << "  output << YAML::BeginMap;" << std::endl;
  for (const auto &field : type_info) {
    const std::string field_name = field.first.as<std::string>();
    outstream << "  output << YAML::Key << \"" << field_name << "\";" << std::endl
              << "  output << YAML::Value << input." << field_name << ";" << std::endl;
  }
  outstream << "  output << YAML::EndMap;" << std::endl
            << "  return output;" << std::endl
            << "}" << std::endl
            << std::endl;
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
    outstream << std::endl << "  " << choice.as<std::string>();
  }
  outstream << std::endl << "};" << std::endl << std::endl;
}

void CodeGenerator::emit_enum_reader(const std::string &type_name, const YAML::Node &type_info) {
  outstream << "inline void operator>>(const YAML::Node &input, " << type_name << " &output) {" << std::endl
            << "  std::string tmp;" << std::endl
            << "  input >> tmp;" << std::endl;
  bool first = true;
  for (const auto &choice : type_info) {
    if (first) {
      outstream << "  ";
    } else {
      outstream << " else ";
    }
    first = false;
    outstream << "if (tmp == \"" << choice << "\") {" << std::endl
              << "    output = " << choice << ";" << std::endl
              << "  }";
  }
  outstream << " else {" << std::endl
            << "    throw YAVL::BadConversionException(input, \"" << type_name << "\");" << std::endl
            << "  }" << std::endl
            << "}" << std::endl;
}

void CodeGenerator::emit_enum_writer(const std::string &type_name, const YAML::Node &type_info) {
  outstream << "inline YAML::Emitter& operator<<(YAML::Emitter &output, const " << type_name << " &input) {"
            << std::endl;
  bool first = true;
  for (const auto &choice : type_info) {
    if (first) {
      outstream << "  ";
    } else {
      outstream << " else ";
    }
    first = false;
    outstream << "if (input == " << choice << ") {" << std::endl
              << "    output << \"" << choice << "\";" << std::endl
              << "  }";
  }
  outstream << std::endl << "  return output;" << std::endl << "}" << std::endl << std::endl;
}

void CodeGenerator::emit_validator() {
  YAML::Node types = spec["Types"];

  outstream << "inline std::vector<std::string> get_types() {" << std::endl << "  return {";
  bool first = true;
  for (const auto &type_def : types) {
    const std::string type_name = type_def.first.as<std::string>();
    if (!first) {
      outstream << ",";
    }
    first = false;
    outstream << std::endl << "    \"" << type_name << "\"";
  }
  outstream << std::endl
            << "  };" << std::endl
            << "}" << std::endl
            << std::endl
            << "inline std::tuple<bool, std::optional<std::string>> validate_simple(const YAML::Node &node, const "
               "std::string type_name) {"
            << std::endl;
  first = true;
  for (const auto &type_def : types) {
    const std::string type_name = type_def.first.as<std::string>();
    if (first) {
      outstream << "  ";
    } else {
      outstream << " else ";
    }
    first = false;
    outstream << "if (type_name == \"" << type_name << "\") {" << std::endl
              << "    return validate<" << type_name << ">(node);" << std::endl
              << "  }";
  }
  outstream << std::endl
            << "  return std::make_tuple(false, std::nullopt);" << std::endl
            << "}" << std::endl
            << std::endl;
}

} // namespace YAVL

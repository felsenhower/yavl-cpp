#pragma once

#include <ostream>
#include <string>
#include <yaml-cpp/yaml.h>

namespace YAVL {

class CodeGenerator {
  public:
    static void emit_header(const YAML::Node &spec, std::ostream &outstream, const bool is_emit_declarations,
        const bool is_emit_readers, const bool is_emit_writers, const bool is_emit_validator);

  protected:
    const YAML::Node &spec;
    std::ostream &outstream;
    const bool is_emit_declarations;
    const bool is_emit_readers;
    const bool is_emit_writers;
    const bool is_emit_validator;

    explicit CodeGenerator(const YAML::Node &spec, std::ostream &outstream, const bool is_emit_declarations,
        const bool is_emit_readers, const bool is_emit_writers, const bool is_emit_validator);
    void emit_header();
    void emit_includes();
    void emit_type(const std::string &type_name, const YAML::Node &type_info);
    void emit_map_declaration(const std::string &type_name, const YAML::Node &type_info);
    void emit_map_reader(const std::string &type_name, const YAML::Node &type_info);
    void emit_map_writer(const std::string &type_name, const YAML::Node &type_info);
    void emit_enum_declaration(const std::string &type_name, const YAML::Node &type_info);
    void emit_enum_reader(const std::string &type_name, const YAML::Node &type_info);
    void emit_enum_writer(const std::string &type_name, const YAML::Node &type_info);
    void emit_alias(const std::string &type_name, const YAML::Node &type_info);
    void emit_validator();
};

} // namespace YAVL

import sys
import argparse
from dataclasses import dataclass
from typing import Any, TextIO
import subprocess
import yaml
import os


@dataclass
class CompilerOptions:
    spec_filestream: TextIO
    header_filestream: TextIO
    validate_spec: bool = True
    emit_declarations: bool = True
    emit_readers: bool = True
    emit_writers: bool = True
    emit_validator: bool = True


class CodeGenerator:
    def __init__(self, spec, options):
        self.spec = spec
        self.options = options
        self.indentation_depth = 2
        self.indentation_level = 0

    def indent(self, levels=1):
        self.indentation_level += levels

    def unindent(self, levels=1):
        self.indentation_level -= levels
        self.indentation_level = max(self.indentation_level, 0)

    def write(self, text="", indent=True, auto_indent=True):
        outstream = self.options.header_filestream
        if auto_indent:
            opening_braces = text.count("{")
            closing_braces = text.count("}")
            levels = opening_braces - closing_braces
            if levels < 0:
                self.unindent(abs(levels))
        indentation = ""
        if indent:
            indentation = " " * (self.indentation_level * self.indentation_depth)
        outstream.write(indentation)
        outstream.write(text)
        if auto_indent and levels > 0:
            self.indent(levels)

    def writeln(self, text="", indent=True, auto_indent=True):
        self.write("{}\n".format(text), indent, auto_indent)

    def emit_header(self):
        options = self.options
        spec = self.spec
        self.emit_includes()
        spec = self.spec
        for type_name, type_info in spec["Types"].items():
            self.emit_type(type_name, type_info)
        if options.emit_validator:
            self.emit_validator()

    def emit_includes(self):
        options = self.options
        spec = self.spec
        self.writeln("#pragma once")
        self.writeln()
        if options.emit_readers or options.emit_writers or options.emit_validator:
            self.writeln('#include "yavl-cpp/convert.h"')
            self.writeln()
        if "ExtraIncludes" in spec:
            for header in spec["ExtraIncludes"]:
                self.writeln("#include {}".format(header))
        self.writeln()

    def emit_type(self, type_name, type_info):
        options = self.options
        if isinstance(type_info, dict):
            if options.emit_declarations:
                self.emit_map_declaration(type_name, type_info)
            if options.emit_readers:
                self.emit_map_reader(type_name, type_info)
            if options.emit_writers:
                self.emit_map_writer(type_name, type_info)
        elif isinstance(type_info, list):
            if options.emit_declarations:
                self.emit_enum_declaration(type_name, type_info)
            if options.emit_readers:
                self.emit_enum_reader(type_name, type_info)
            if options.emit_writers:
                self.emit_enum_writer(type_name, type_info)
        else:
            if options.emit_declarations:
                self.emit_alias(type_name, type_info)

    def emit_map_declaration(self, type_name, type_info):
        self.writeln("struct {} {{".format(type_name))
        for field_name, field_type in type_info.items():
            bracket_pos = field_type.find("[")
            is_array_type = bracket_pos != -1
            if is_array_type:
                base_type = field_type[0:bracket_pos]
                array_length = field_type[bracket_pos:]
                self.writeln("{} {}{};".format(base_type, field_name, array_length))
            else:
                self.writeln("{} {};".format(field_type, field_name))
        self.writeln("};")
        self.writeln()

    def emit_map_reader(self, type_name, type_info):
        self.writeln(
            "inline void operator>>(const YAML::Node &input, {} &output) {{".format(
                type_name
            )
        )
        self.writeln("const std::unordered_map<std::string, bool> keys = {")
        num_fields = len(type_info)
        for i, (field_name, field_type) in enumerate(type_info.items()):
            is_required = not field_type.startswith("std::optional<")
            is_last = i == (num_fields - 1)
            self.writeln(
                '{{"{}", {}}}{}'.format(
                    field_name,
                    "true" if is_required else "false",
                    "," if not is_last else "",
                )
            )
        self.writeln("};")
        self.writeln("for (const auto &[key, is_required] : keys) {")
        self.writeln("if (is_required && !input[key]) {")
        self.writeln('throw YAVL::MissingKeyException("{}", key);'.format(type_name))
        self.writeln("}")
        self.writeln("}")
        self.writeln("for (const auto &it : input) {")
        self.writeln("const std::string key = it.first.as<std::string>();")
        self.writeln("if (!keys.contains(key)) {")
        self.writeln(
            'throw YAVL::SuperfluousKeyException("{}", key);'.format(type_name)
        )
        self.writeln("}")
        self.writeln("}")
        for field_name in type_info:
            self.writeln('input["{0}"] >> output.{0};'.format(field_name))
        self.writeln("}")
        self.writeln()

    def emit_map_writer(self, type_name, type_info):
        self.writeln(
            "inline YAML::Emitter& operator<<(YAML::Emitter &output, const {} &input) {{".format(
                type_name
            )
        )
        self.writeln("output << YAML::BeginMap;")
        for field_name in type_info:
            self.writeln('output << YAML::Key << "{}";'.format(field_name))
            self.writeln("output << YAML::Value << input.{};".format(field_name))
        self.writeln("output << YAML::EndMap;")
        self.writeln("return output;")
        self.writeln("}")
        self.writeln()

    def emit_enum_declaration(self, type_name, type_info):
        self.writeln("enum {} {{".format(type_name))
        num_choices = len(type_info)
        for i, choice in enumerate(type_info):
            is_last = i == (num_choices - 1)
            self.writeln("{}{}".format(choice, "," if not is_last else ""))
        self.writeln("};")
        self.writeln()

    def emit_enum_reader(self, type_name, type_info):
        self.writeln(
            "inline void operator>>(const YAML::Node &input, {} &output) {{".format(
                type_name
            )
        )
        self.writeln("std::string tmp;")
        self.writeln("input >> tmp;")
        num_choices = len(type_info)
        for i, choice in enumerate(type_info):
            is_first = i == 0
            is_last = i == (num_choices - 1)
            self.writeln('if (tmp == "{}") {{'.format(choice), indent=is_first)
            self.writeln("output = {};".format(choice))
            self.write("}")
            self.write(" else ", indent=False)
        self.writeln("{", indent=False)
        self.writeln(
            'throw YAVL::BadConversionException(input, "{}");'.format(type_name)
        )
        self.writeln("}")
        self.writeln("}")
        self.writeln()

    def emit_enum_writer(self, type_name, type_info):
        self.writeln(
            "inline YAML::Emitter& operator<<(YAML::Emitter &output, const {} &input) {{".format(
                type_name
            )
        )
        num_choices = len(type_info)
        for i, choice in enumerate(type_info):
            is_first = i == 0
            is_last = i == (num_choices - 1)
            self.writeln("if (input == {}) {{".format(choice), indent=is_first)
            self.writeln('output << "{}";'.format(choice))
            self.write("}")
            if not is_last:
                self.write(" else ", indent=False)
        self.writeln(indent=False)
        self.writeln("return output;")
        self.writeln("}")
        self.writeln()

    def emit_alias(self, type_name, type_info):
        field_type = str(type_info)
        bracket_pos = field_type.find("[")
        is_array_type = bracket_pos != -1
        if is_array_type:
            base_type = field_type[0:bracket_pos]
            array_length = field_type[bracket_pos:]
            self.writeln("typedef {} {}{};".format(base_type, type_name, array_length))
        else:
            self.writeln("typedef {} {};".format(field_type, type_name))
        self.writeln()

    def emit_validator(self):
        self.writeln("inline std::vector<std::string> get_types() {")
        self.writeln("return {")
        types = self.spec["Types"]
        num_types = len(types)
        for i, type_name in enumerate(types):
            is_last = i == (num_types - 1)
            self.writeln('"{}"{}'.format(type_name, "," if not is_last else ""))
        self.writeln("};")
        self.writeln("}")
        self.writeln()
        self.writeln(
            "inline std::tuple<bool, std::optional<std::string>> validate_simple(const YAML::Node &node, const std::string type_name) {"
        )
        for i, type_name in enumerate(types):
            is_first = i == 0
            is_last = i == (num_types - 1)
            self.writeln('if (type_name == "{}") {{'.format(type_name), indent=is_first)
            self.writeln("return validate<{}>(node);".format(type_name))
            self.write("}")
            if not is_last:
                self.write(" else ", indent=False)
        self.writeln(indent=False)
        self.writeln("return std::make_tuple(false, std::nullopt);")
        self.writeln("}")
        self.writeln()


if __name__ == "__main__":
    sys.exit("Error: This module is meant to be imported!")

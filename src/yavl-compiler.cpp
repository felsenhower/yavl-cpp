#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

#include "yavl-cpp/yavl.h"

class Compiler {
  public:
    explicit Compiler(int argc, char **argv);
    void generate();

  private:
    static const inline std::map<const std::string, const std::string &> option_descriptions = {
        {"--help",                 "Print this help."                                        },
        {"--no-emit-declarations", "Don't emit type declarations."                           },
        {"--no-emit-readers",      "Don't emit code to convert from YAML to generated types."},
        {"--no-emit-writers",      "Don't emit code to convert from generated types to YAML."},
        {"--no-emit-validator",    "Don't emit code to validate a YAML document."            }
    };

    const std::string &app_name;
    const std::vector<std::string> args;
    YAML::Node spec;
    std::ofstream outfstream;
    bool emit_declarations = true;
    bool emit_readers = true;
    bool emit_writers = true;
    bool emit_validator = true;
    void parse_args();
    void parse_error(const std::string &error_message);
    void usage(std::ostream &outstream, const int exit_code = EXIT_SUCCESS);
};

Compiler::Compiler(int argc, char **argv) : app_name(argv[0]), args(std::vector<std::string>(argv + 1, argv + argc)) {
  parse_args();
}

void Compiler::parse_args() {
  if (args.size() == 1 && args[0] == "--help") {
    usage(std::cout);
  }
  if (args.size() < 2) {
    parse_error("Not enough arguments.");
  }
  std::vector<std::string> args_copy = args;
  std::string header_filename = args_copy.back();
  args_copy.pop_back();
  std::string spec_filename = args_copy.back();
  args_copy.pop_back();
  for (const auto &option : args_copy) {
    if (!option_descriptions.contains(option)) {
      parse_error("Unknown option \"" + option + "\"");
    }
    if (option == "--no-emit-declarations") {
      emit_declarations = false;
    } else if (option == "--no-emit-readers") {
      emit_readers = false;
    } else if (option == "--no-emit-writers") {
      emit_writers = false;
    } else if (option == "--no-emit-validator") {
      emit_validator = false;
    }
  }
  try {
    spec = YAML::LoadFile(spec_filename);
  } catch (const YAML::Exception &e) {
    parse_error("Couldn't read spec \"" + spec_filename + "\": \"" + e.what() + "\"");
  }
  outfstream.open(header_filename);
  if (!outfstream.is_open()) {
    parse_error("Couldn't open header file \"" + header_filename + "\" for write access.");
  }
}

void Compiler::parse_error(const std::string &error_message) {
  std::cerr << "Error: " << error_message << std::endl << std::endl;
  usage(std::cerr, EXIT_FAILURE);
}

void Compiler::usage(std::ostream &outstream, const int exit_code) {
  std::size_t max_option_length =
      std::max_element(option_descriptions.begin(), option_descriptions.end(), [](const auto &lhs, const auto &rhs) {
        return (lhs.first.length() < rhs.first.length());
      })->first.length();
  outstream << "Usage: " << app_name << " [OPTION...] SPEC HEADER" << std::endl
            << "Generate a C/C++ HEADER file from a YAVL SPEC." << std::endl
            << std::endl
            << "Options:" << std::endl;
  for (const auto &[option, description] : option_descriptions) {
    outstream << "  " << std::left << std::setw(max_option_length + 2) << option << description << std::endl;
  }
  outstream << std::endl;
  exit(exit_code);
}

void Compiler::generate() {
  YAVL::CodeGenerator::emit_header(spec, outfstream, emit_declarations, emit_readers, emit_writers, emit_validator);
}

int main(int argc, char **argv) {
  Compiler compiler(argc, argv);
  compiler.generate();
  return EXIT_SUCCESS;
}

#include <fstream>
#include <iostream>
#include <string>
#include <yaml-cpp/yaml.h>

#include "yavl-cpp/yatc.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Not enough arguments!\n";
    return EXIT_FAILURE;
  }
  const std::string spec_filename = argv[1];
  YAML::Node spec;
  try {
    spec = YAML::LoadFile(spec_filename);
  } catch (const YAML::Exception &e) {
    std::cerr << "Error reading spec: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  const std::string outfilename(argv[2]);
  std::ofstream outstream(outfilename);

  const bool emit_declarations = true;
  const bool emit_readers = true;
  const bool emit_writers = true;

  YAVL::CodeGenerator::emit_header(spec, outstream, emit_declarations, emit_readers, emit_writers);
}

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
  const std::string grammar_filename = argv[1];
  
  YAML::Node gr;
  try {
    gr = YAML::LoadFile(grammar_filename);
  } catch (const YAML::Exception &e) {
    std::cerr << "Error reading grammar: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  const std::string topname(argv[2]);
  const std::string outfile = topname + ".h";
  std::ofstream hf(outfile);
  YAVL::DataBinderGen::emit_header(gr, topname, hf);
}

#include <fstream>
#include <iostream>
#include <yaml-cpp/yaml.h>

#include "yavl.h"

int main(int argc, char **argv) {
  const std::string grammar_filename = argv[1];
  YAML::Node gr;
  try {
    gr = YAML::LoadFile(grammar_filename);
  } catch (const YAML::Exception &e) {
    std::cerr << "Error reading grammar: " << e.what() << "\n";
    return 1;
  }

  const std::string doc_filename = argv[2];
  YAML::Node doc;
  try {
    doc = YAML::LoadFile(doc_filename);
  } catch (const YAML::Exception &e) {
    std::cerr << "Error reading document: " << e.what() << "\n";
    return 2;
  }

  YAVL::Validator yavl(gr, doc);
  bool ok = yavl.validate();
  if (!ok) {
    std::cout << "ERRORS FOUND: " << std::endl << std::endl;
    std::cout << yavl.get_errors();
  }
  return !ok;
}

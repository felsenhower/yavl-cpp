#include <fstream>
#include <iostream>
#include <string>
#include <yaml-cpp/yaml.h>

#include "yatc.h"

int main(int argc, char **argv) {
  YAML::Node gr;
  const std::string grammar_filename = argv[1];
  try {
    gr = YAML::LoadFile(grammar_filename);
  } catch (const YAML::Exception &e) {
    std::cerr << "Error reading grammar: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  std::string topname(argv[2]);
  YAVL::DataBinderGen yatc(gr, topname);

  std::ofstream hf;
  hf.open((topname + ".h").c_str());
  yatc.emit_header(hf);
  hf.close();

  std::ofstream rf;
  rf.open((topname + ".cpp").c_str());
  yatc.emit_reader(rf);
  yatc.emit_dumper(rf);
  rf.close();
}

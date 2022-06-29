#include <fstream>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include "yavl.h"

using namespace std;

int main(int argc, char **argv)
{
  const string grammar_filename = argv[1];
  YAML::Node gr;
  try {
    gr = YAML::LoadFile(grammar_filename);
  } catch(const YAML::Exception& e) {
    std::cerr << "Error reading grammar: " << e.what() << "\n";
    return 1;
  }

  const string doc_filename = argv[2];
  YAML::Node doc;
  try {
    doc = YAML::LoadFile(doc_filename);
  } catch(const YAML::Exception& e) {
    std::cerr << "Error reading document: " << e.what() << "\n";
    return 2;
  }

  YAVL::Validator yavl(gr, doc);
  bool ok = yavl.validate();
  if (!ok) {
    cout << "ERRORS FOUND: " << endl << endl;
    cout << yavl.get_errors();
  }
  return !ok;
}

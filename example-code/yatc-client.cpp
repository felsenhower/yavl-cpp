#include <fstream>
#include <iostream>
#include "top.h"

using namespace std;

int main(int argc, char **argv)
{
  
  Top top;
  const string doc_filename = argv[1];
  try {
    YAML::Node doc = YAML::LoadFile(doc_filename);

    // read YAML file into our data structures
    doc >> top;

    // write out our data structure as a YAML
    YAML::Emitter out;
    // first build the in-memory YAML tree
    out << top;

    // dump it to disk
    cout << out.c_str() << endl;
  } catch(const YAML::Exception& e) {
    std::cerr << e.what() << "\n";
  }
  return 0;
}

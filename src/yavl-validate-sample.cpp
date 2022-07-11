#include <dlfcn.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <yaml-cpp/yaml.h>

#include "yavl-cpp/runtime.h"

using YAVL::SymbolTable;
typedef SymbolTable (*get_symbols_ptr)();

void usage(const std::string &app_name) {
  std::cerr << "Usage: " << app_name << " DOC LIB TYPENAME" << std::endl;
}

int main(int argc, char **argv) {
  if (argc < 4) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  const std::string doc_filename = argv[1];
  YAML::Node doc;
  try {
    doc = YAML::LoadFile(doc_filename);
  } catch (const YAML::Exception &e) {
    std::cerr << "Error while parsing document: \"" << e.what() << "\"" << std::endl;
    return EXIT_FAILURE;
  }
  const std::string lib_filename = argv[2];
  const std::unique_ptr<void, std::function<void(void *)>> handle(
      dlopen(lib_filename.c_str(), RTLD_LAZY), [](void *ptr) { dlclose(ptr); });
  if (!handle.get()) {
    std::cerr << "Error: \"" << dlerror() << "\"" << std::endl;
    return EXIT_FAILURE;
  }
  get_symbols_ptr get_symbols = (get_symbols_ptr)dlsym(handle.get(), "get_symbols");
  const char *error = dlerror();
  if (!get_symbols || error) {
    std::cerr << "Error: \"" << error << "\"" << std::endl;
    return EXIT_FAILURE;
  }
  SymbolTable symbols = get_symbols();
  const std::string type_name = argv[3];
  std::vector<std::string> types = symbols.get_types();
  if (std::find(types.begin(), types.end(), type_name) == types.end()) {
    std::cerr << "Invalid type: \"" << type_name << "\"" << std::endl;
    return EXIT_FAILURE;
  }
  const auto &[ok, error_message] = symbols.validate_simple(doc, type_name);
  if (ok) {
    std::cout << "Validation successful!" << std::endl;
  } else {
    std::cerr << "Validation failed!" << std::endl
              << "Error: " << std::endl
              << "  " << error_message.value_or("(Unknown)") << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

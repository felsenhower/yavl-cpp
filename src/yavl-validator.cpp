#include <dlfcn.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <yaml-cpp/yaml.h>

typedef std::tuple<bool, std::optional<std::string>> validation_result;
typedef validation_result (*validation_function)(const YAML::Node &node, const std::string type_name);

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
  validation_function validate = (validation_function)dlsym(handle.get(), "validate_simple");
  const char *error = dlerror();
  if (!validate || error) {
    std::cerr << "Error: \"" << error << "\"" << std::endl;
    return EXIT_FAILURE;
  }
  const std::string type_name = argv[3];
  const auto &[ok, error_message] = validate(doc, type_name);
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

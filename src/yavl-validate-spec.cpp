#include <iostream>

#include "yavl-cpp/spec.h"

void usage(const std::string &app_name) {
  std::cerr << "Usage: " << app_name << " SPEC" << std::endl
            << std::endl
            << "Check, if the given yaml document is a valid YAVL spec." << std::endl;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  const std::string spec_filename = argv[1];
  YAML::Node spec;
  try {
    spec = YAML::LoadFile(spec_filename);
  } catch (const YAML::Exception &e) {
    std::cerr << "Error while parsing document: \"" << e.what() << "\"" << std::endl;
    return EXIT_FAILURE;
  }
  const auto &[valid_spec, error_message] = validate<SpecType>(spec);
  if (!valid_spec) {
    std::cerr << "Validation failed!" << std::endl
              << "Error: " << std::endl
              << "  " << error_message.value_or("(Unknown)") << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "Validation successful!" << std::endl;
  return EXIT_SUCCESS;
}

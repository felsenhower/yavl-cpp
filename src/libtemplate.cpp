#include "spec.h"

extern "C" {

YAVL::SymbolTable get_symbols() {
  const YAVL::SymbolTable symbols = {.validate_simple = validate_simple, .get_types = get_types};
  return symbols;
}
}

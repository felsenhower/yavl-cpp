# yavl-cpp

`yavl-cpp` is a tool to auto-generate C/C++ code containing struct declarations, data bindings (methods to convert YAML to generated structs and vice versa) and validators from a specification written in a subset of YAML ("YAVL").

In principle, `yavl-cpp` is similar to Google Protobuf: You specify data structures in an abstract language and generate C++ code from that.
Some key differences are:
- Structure definitions in `yavl-cpp` are done via YAML documents.
- `yavl-cpp` does not perform any kind of serialization.
- Most of the validation is done either during parsing (by `yaml-cpp`) or in compilation (by `g++`). 
`yavl-compiler` doesn't verify any of the types you specified. All types that `yaml-cpp` and the compiler understand, are valid.

## Usage

YAVL specifications are simple:

```yaml
Types:
    Size: 
        - big
        - small
    A:
        a1: std::string
        a2: std::string
    Pieces:
        a: std::vector<A>
        b: std::vector<int>
    Header:
        name: std::string
        version: std::string
        size: Size
        pieces: Pieces
    Top:
        header: Header
```

All types must be declared under the key `Types`.

A type will be declared an `enum` if its corresponding YAML node is a sequence, and a `struct` if its YAML node is a map. 

Running the YAVL compiler creates a header file.

```bash
$ make
$ ./yavl-compiler --no-emit-readers --no-emit-writers --no-emit-validator examples/example_2_spec.yaml simple.h
```

The resulting header file `simple.h` is then

```C++
#pragma once

enum Size {
  big,
  small
};

struct Pieces {
  std::vector<std::string> a;
  std::vector<uint64_t> b;
};

struct Header {
  std::string name;
  std::string version;
  Size size;
  Pieces pieces;
};

struct Top {
  Header header;
};
```

By default, `yavl-compiler` creates declarations, readers, writers, and validators. Except for the declarations, this functionality depends on `yaml-cpp`.

```bash
$ ./yavl-compiler examples/example_2_spec.yaml simple.h
```

The `simple.h` will then look like this:

```C
#pragma once

#include <yaml-cpp/yaml.h>
#include "yavl-cpp/convert.h"

enum Size {
  big,
  small
};

inline void operator>>(const YAML::Node &input, Size &output) {
  std::string tmp;
  input >> tmp;
  if (tmp == "big") {
    output = big;
  } else if (tmp == "small") {
    output = small;
  }
}
inline YAML::Emitter& operator<<(YAML::Emitter &output, const Size &input) {
  if (input == big) {
    output << "big";
  } else if (input == small) {
    output << "small";
  }
  return output;
}

struct Pieces {
  std::vector<std::string> a;
  std::vector<uint64_t> b;
};

inline void operator>>(const YAML::Node &input, Pieces &output) {
  input["a"] >> output.a;
  input["b"] >> output.b;
}

inline YAML::Emitter& operator<<(YAML::Emitter &output, const Pieces &input) {
  output << YAML::BeginMap;
  output << YAML::Key << "a";
  output << YAML::Value << input.a;
  output << YAML::Key << "b";
  output << YAML::Value << input.b;
  output << YAML::EndMap;
  return output;
}

struct Header {
  std::string name;
  std::string version;
  Size size;
  Pieces pieces;
};

inline void operator>>(const YAML::Node &input, Header &output) {
  input["name"] >> output.name;
  input["version"] >> output.version;
  input["size"] >> output.size;
  input["pieces"] >> output.pieces;
}

inline YAML::Emitter& operator<<(YAML::Emitter &output, const Header &input) {
  output << YAML::BeginMap;
  output << YAML::Key << "name";
  output << YAML::Value << input.name;
  output << YAML::Key << "version";
  output << YAML::Value << input.version;
  output << YAML::Key << "size";
  output << YAML::Value << input.size;
  output << YAML::Key << "pieces";
  output << YAML::Value << input.pieces;
  output << YAML::EndMap;
  return output;
}

struct Top {
  Header header;
};

inline void operator>>(const YAML::Node &input, Top &output) {
  input["header"] >> output.header;
}

inline YAML::Emitter& operator<<(YAML::Emitter &output, const Top &input) {
  output << YAML::BeginMap;
  output << YAML::Key << "header";
  output << YAML::Value << input.header;
  output << YAML::EndMap;
  return output;
}

inline std::tuple<bool, std::optional<std::string>> validate(const YAML::Node &node, const std::string type_name) {
  if (type_name == "Size") {
    return validate<Size>(node);
  } else if (type_name == "Pieces") {
    return validate<Pieces>(node);
  } else if (type_name == "Header") {
    return validate<Header>(node);
  } else if (type_name == "Top") {
    return validate<Top>(node);
  }
  return std::make_tuple(false, std::nullopt);
}
```

Load a YAML document into a generated data structure like this:

```C++
#include "yaml-cpp/yaml.h"
#include "simple.h"

int main() {
    YAML::Node doc = YAML::LoadFile("example.yaml");
    Top top;
    doc >> top;
}
```

When including this header into your C/C++ project, don't forget to add a `-I/path/to/yavl-cpp/include/` to your `CFLAGS` and to add `yaml-cpp`:

```bash
$ g++ -Iinclude $(pkg-config --libs yaml-cpp) simple.cpp
```

If you wish to create header files that are valid C code, simply don't emit writers, readers, and validators, like in the example above, and don't use any types that aren't available in C (like `std::string`, `std::vector`, ...).

`yavl-compiler` does not check the correctness of the types you specified in your YAML file. Any mistakes will only be uncovered later when you include the header file into your project.

## Validation

Using the example above, you can also validate YAML documents:

```C++
const auto &[ok, error_message] = validate<Top>(doc);
// or
const auto &[ok, error_message] = validate(doc, "Top");
```

`ok` is a `bool` which is `true` if `doc` is a valid YAML representation of the `Top` type. 
`error_message` is an `std::optional<std::string>` that holds the error message if `ok == false`. Otherwise, `error_message` will be `std::nullopt`.

You can use the script `validate.sh` to validate a YAML document against a YAVL specification:

```bash
$ ./validate.sh examples/example_1_sample_correct.yaml examples/example_1_spec.yaml Top
```

> :warning: **Attention: This is potentially dangerous!** `validate.sh` will compile your spec to a header, use `g++` to create a dynamic library, and `yavl-validator` will execute binary code from this library without any checks. Never execute this script in a working environment you don't trust 100%! This is purely for demonstration purposes.

## Dependencies

- `yaml-cpp-0.7.0`
- `make`
- `g++`
- `pkg-config`
- `bash`

## History

The original `yavl-cpp` was written by Ranganathan Sankaralingam. It has been auto-converted using `git-svn` from [Google Code Archive](https://code.google.com/archive/p/yavl-cpp/). The original branches `trunk` and `yatc` are archived.

The `master` branch is based on the `yatc` branch. Since then, the project has developed in quite a different direction.

The new YAVL syntax is inspired by Pieter Lexis' [`yavl-cpp` fork](https://github.com/pieterlexis/yavl-cpp/commit/536ddc3f77231d0541c1103727d06482e1b89b43).

## Authors

- Ranganathan Sankaralingam
- Ruben Felgenhauer
- Pieter Lexis
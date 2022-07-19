# yavl-cpp

`yavl-cpp` is a tool to auto-generate C/C++ code containing struct declarations, data bindings (methods to convert YAML to generated structs and vice versa) and validators from a specification written in a subset of YAML ("YAVL").

In principle, `yavl-cpp` is similar to Google Protobuf: You specify data structures in an abstract language and generate C++ code from that.
Some key differences are:
- Structure definitions in `yavl-cpp` are done via YAML documents.
- `yavl-cpp` does not perform any kind of serialization.
- Most of the validation is done either during parsing (by `yaml-cpp`) or in compilation (by `g++`). 
`yavl-compile` doesn't verify any of the types you specified. All types that `yaml-cpp` and the compiler understand, are valid.

## Usage

The full YAVL language specification can be found in the [Wiki](../../wiki/Language-Specification).
Here is an example:

```yaml
Types:
    String: std::string
    EnumType:
        - first_choice
        - second_choice
    TopType:
        my_vec: std::vector<std::string>
        my_enum: EnumType
        my_int: int
        my_str: String
```

Here is an example of a YAML document that matches the type `TopType` that is declared in this spec:

```YAML
my_vec: ["monkey", "bread", "tree"]
my_enum: first_choice
my_int: 5
my_str: foo
```

The YAVL compiler can be built and run like this:

```bash
$ git clone https://github.com/felsenhower/yavl-cpp
$ cd yavl-cpp
$ make
$ ./yavl-compile examples/simple_spec.yaml simple.h
```

The resulting C++ header file `simple.h` will contain the following declarations (among other things):

```C++
typedef std::string String;

enum EnumType {
  first_choice,
  second_choice
};

struct TopType {
  std::vector<std::string> my_vec;
  EnumType my_enum;
  int my_int;
  String my_str;
};
```

<details>
  <summary>Full <code>simple.h</code> (click to expand!)</summary>

```C++
#pragma once

#include <yaml-cpp/yaml.h>
#include "yavl-cpp/runtime.h"

#include <vector>
#include <string>

typedef std::string String;

enum EnumType {
  first_choice,
  second_choice
};

inline void operator>>(const YAML::Node &input, EnumType &output) {
  std::string tmp;
  input >> tmp;
  if (tmp == "first_choice") {
    output = first_choice;
  } else if (tmp == "second_choice") {
    output = second_choice;
  } else {
    throw YAVL::BadConversionException(input, "EnumType");
  }
}
inline YAML::Emitter& operator<<(YAML::Emitter &output, const EnumType &input) {
  if (input == first_choice) {
    output << "first_choice";
  } else if (input == second_choice) {
    output << "second_choice";
  }
  return output;
}

struct TopType {
  std::vector<std::string> my_vec;
  EnumType my_enum;
  int my_int;
  String my_str;
};

inline void operator>>(const YAML::Node &input, TopType &output) {
  const std::set<std::string> keys = {
    "my_vec",
    "my_enum",
    "my_int",
    "my_str"
  };
  for (const auto &key : keys) {
    if (!input[key]) {
      throw YAVL::MissingKeyException("TopType", key);
    }
  }
  for (const auto &it : input) {
    const std::string key = it.first.as<std::string>();
    if (!keys.contains(key)) {
      throw YAVL::SuperfluousKeyException("TopType", key);
    }
  }
  input["my_vec"] >> output.my_vec;
  input["my_enum"] >> output.my_enum;
  input["my_int"] >> output.my_int;
  input["my_str"] >> output.my_str;
}

inline YAML::Emitter& operator<<(YAML::Emitter &output, const TopType &input) {
  output << YAML::BeginMap;
  output << YAML::Key << "my_vec";
  output << YAML::Value << input.my_vec;
  output << YAML::Key << "my_enum";
  output << YAML::Value << input.my_enum;
  output << YAML::Key << "my_int";
  output << YAML::Value << input.my_int;
  output << YAML::Key << "my_str";
  output << YAML::Value << input.my_str;
  output << YAML::EndMap;
  return output;
}

inline std::vector<std::string> get_types() {
  return {
    "String",
    "EnumType",
    "TopType"
  };
}

inline std::tuple<bool, std::optional<std::string>> validate_simple(const YAML::Node &node, const std::string type_name) {
  if (type_name == "String") {
    return validate<String>(node);
  } else if (type_name == "EnumType") {
    return validate<EnumType>(node);
  } else if (type_name == "TopType") {
    return validate<TopType>(node);
  }
  return std::make_tuple(false, std::nullopt);
}
```

</details>

By default, `yavl-compile` creates declarations, readers, writers, and validators. Except for the declarations, this functionality depends on `yaml-cpp`. See the [Wiki](../../wiki/Command-Line-Interface) for the complete Command Line Interface description.

When including this header into your C/C++ project, don't forget to add a `-I/path/to/yavl-cpp/include/` to your `CFLAGS` and to add `yaml-cpp`:

```bash
$ g++ -Iinclude $(pkg-config --libs yaml-cpp) simple.cpp
```

If you wish to create header files that are valid C code, simply don't emit writers, readers, and validators, like in the example above, and don't use any types that aren't available in C (like `std::string`, `std::vector`, ...).

`yavl-compile` does not check the correctness of the types you specified in your YAML file. Any mistakes will only be uncovered later when you include the header file into your project.

## Validation

Using the example above, you can also validate YAML documents:

```C++
const auto &[ok, error_message] = validate<TopType>(doc);
// or
const auto &[ok, error_message] = validate(doc, "TopType");
```

`ok` is a `bool` which is `true` if `doc` is a valid YAML representation of the `Top` type. 
`error_message` is an `std::optional<std::string>` that holds the error message if `ok == false`. Otherwise, `error_message` will be `std::nullopt`.

You can use the script `validate.sh` to validate a YAML document against a YAVL specification:

```bash
$ ./validate.sh examples/simple_sample_correct.yaml examples/simple_spec.yaml TopType
Compiling spec...
Compiling shared object...
Validating...
Validation successful!
```

> :warning: **Attention: This is potentially dangerous!** `validate.sh` will compile your spec to a header, use `g++` to create a dynamic library, and `yavl-validate-sample` will execute binary code from this library without any checks. Never execute this script in a working environment you don't trust 100%! This is purely for demonstration purposes.

## Dependencies

- `yaml-cpp-0.7.0`
- `make`
- `g++`
- `pkg-config`
- `bash`
- `python3`
- `pyyaml`

## Contributors

The original `yavl-cpp` was written by **[Ranganathan Sankaralingam](https://github.com/psranga)**. It has been auto-converted using `git-svn` from [Google Code Archive](https://code.google.com/archive/p/yavl-cpp/). The original branches `trunk` and `yatc` are archived.

The `master` branch is based on the `yatc` branch. Since then, the project was developed in quite a different direction (aside from the name and the general idea, nothing of the original code is left) by me (**[Ruben Felgenhauer](https://github.com/felsenhower)**).

The new YAVL syntax is inspired by the [`yavl-cpp` fork](https://github.com/pieterlexis/yavl-cpp/commit/536ddc3f77231d0541c1103727d06482e1b89b43) by **[Pieter Lexis](https://github.com/pieterlexis)**.

This code uses some [C++ classes](include/tsl) by **[Thibaut Goetghebuer-Planchon](https://github.com/Tessil)**.

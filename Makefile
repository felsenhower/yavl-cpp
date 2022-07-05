.PHONY: default all clean test yatc-client

YAML_CPP_SUBMODULE_PATH := $(shell git config -f .gitmodules submodule.yaml-cpp.path)
YAML_CPP_PATH := $(YAML_CPP_SUBMODULE_PATH)
YAML_CPP_SOURCES := $(addprefix $(YAML_CPP_PATH)/src/,binary.cpp convert.cpp depthguard.cpp directives.cpp emit.cpp emitfromevents.cpp emitter.cpp emitterstate.cpp emitterutils.cpp exceptions.cpp exp.cpp memory.cpp nodebuilder.cpp node.cpp node_data.cpp nodeevents.cpp null.cpp ostream_wrapper.cpp parse.cpp parser.cpp regex_yaml.cpp scanner.cpp scanscalar.cpp scantag.cpp scantoken.cpp simplekey.cpp singledocparser.cpp stream.cpp tag.cpp)

BUILD_DIR := build

YATC_SOURCES := src/yatc.cpp
YAVL_COMPILER_SOURCES := src/yavl-compiler.cpp $(YATC_SOURCES) $(YAML_CPP_SOURCES)
YATC_CLIENT_SOURCES := src/yatc-client.cpp $(YATC_SOURCES) $(YAML_CPP_SOURCES)

CXX = g++
INCLUDE_DIRS := $(YAML_CPP_PATH)/include include $(BUILD_DIR)
CXXFLAGS = -O3 -std=c++20 -Wall -Werror -Wpedantic -Wno-restrict -Wno-dangling-pointer
CXXFLAGS += $(addprefix -I,$(INCLUDE_DIRS))

default: yavl-compiler $(YAML_CPP_SOURCES)

all: test

test: yavl-compiler $(YAML_CPP_SOURCES)
	@echo -e '\n>> Testing yavl-compiler...\n'
	$(MAKE) $(BUILD_DIR)/top.h
	$(MAKE) $(BUILD_DIR)/yatc-client
	@echo -e '\n>> Testing yatc-client...\n'
	$(BUILD_DIR)/yatc-client examples/example_1_sample_correct.yaml
	@echo -e '\n>> All tests finished successfully.\n'

yavl-compiler: $(BUILD_DIR)/yavl-compiler
	ln -sf $^ $@

$(BUILD_DIR)/yavl-compiler: $(addprefix $(BUILD_DIR)/,$(YAVL_COMPILER_SOURCES:.cpp=.o))
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@

$(BUILD_DIR)/yatc-client: $(addprefix $(BUILD_DIR)/,$(YATC_CLIENT_SOURCES:.cpp=.o))
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $^

$(BUILD_DIR)/top.h: yavl-compiler
	./yavl-compiler examples/example_1_spec.yaml $(BUILD_DIR)/top.h


$(YAML_CPP_SUBMODULE_PATH)/%::
	git submodule update --init $(YAML_CPP_SUBMODULE_PATH)

clean:
	$(RM) -r $(BUILD_DIR)

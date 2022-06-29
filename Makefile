.PHONY: default all clean test checker tc yatc-client

YAML_CPP_SUBMODULE_PATH := $(shell git config -f .gitmodules submodule.yaml-cpp.path)
YAML_CPP_PATH := $(YAML_CPP_SUBMODULE_PATH)
YAML_CPP_SOURCES := $(addprefix $(YAML_CPP_PATH)/src/,binary.cpp convert.cpp depthguard.cpp directives.cpp emit.cpp emitfromevents.cpp emitter.cpp emitterstate.cpp emitterutils.cpp exceptions.cpp exp.cpp memory.cpp nodebuilder.cpp node.cpp node_data.cpp nodeevents.cpp null.cpp ostream_wrapper.cpp parse.cpp parser.cpp regex_yaml.cpp scanner.cpp scanscalar.cpp scantag.cpp scantoken.cpp simplekey.cpp singledocparser.cpp stream.cpp tag.cpp)

BUILD_DIR := build

YAVL_SOURCES := src/yavl.cpp
YATC_SOURCES := src/yatc.cpp
CHECKER_SOURCES := example-code/checker.cpp $(YAVL_SOURCES) $(YAML_CPP_SOURCES)
TC_SOURCES := example-code/tc.cpp $(YATC_SOURCES) $(YAML_CPP_SOURCES)
YATC_CLIENT_SOURCES := example-code/yatc-client.cpp $(YATC_SOURCES) $(YAML_CPP_SOURCES) $(BUILD_DIR)/top.cpp

CXX = g++
INCLUDE_DIRS := $(YAML_CPP_PATH)/include src $(BUILD_DIR)
CXXFLAGS = -O3
CXXFLAGS += $(addprefix -I,$(INCLUDE_DIRS))

default: checker tc $(YAML_CPP_SOURCES)

all: test

test: checker tc $(YAML_CPP_SOURCES)
	@echo -e '\n>> Testing checker...\n'
	! $(BUILD_DIR)/checker example-specs/gr3.yaml example-specs/y0.gr3.yaml
	@echo -e '\n>> Testing tc...\n'
	$(MAKE) $(BUILD_DIR)/top.cpp
	$(MAKE) $(BUILD_DIR)/top.h
	$(MAKE) $(BUILD_DIR)/yatc-client
	@echo -e '\n>> Testing yatc-client...\n'
	$(BUILD_DIR)/yatc-client example-specs/y0.gr4.yaml
	@echo -e '\n>> All tests finished successfully.\n'

checker: $(BUILD_DIR)/checker

$(BUILD_DIR)/checker: $(addprefix $(BUILD_DIR)/,$(CHECKER_SOURCES:.cpp=.o))
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@

tc: $(BUILD_DIR)/tc

$(BUILD_DIR)/tc: $(addprefix $(BUILD_DIR)/,$(TC_SOURCES:.cpp=.o))
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@

$(BUILD_DIR)/yatc-client: $(addprefix $(BUILD_DIR)/,$(YATC_CLIENT_SOURCES:.cpp=.o))
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $^

$(BUILD_DIR)/top.cpp $(BUILD_DIR)/top.h: $(BUILD_DIR)/tc
	cd $(BUILD_DIR) ; ./tc $(shell realpath --relative-to=$(BUILD_DIR) .)/example-specs/gr4.yaml top


$(YAML_CPP_SUBMODULE_PATH)/%::
	git submodule update --init $(YAML_CPP_SUBMODULE_PATH)

clean:
	$(RM) -r $(BUILD_DIR)

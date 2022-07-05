.PHONY: default all clean test yatc-client

BUILD_DIR := build

YATC_SOURCES := src/yatc.cpp
YAVL_COMPILER_SOURCES := src/yavl-compiler.cpp $(YATC_SOURCES)
YATC_CLIENT_SOURCES := src/yatc-client.cpp $(YATC_SOURCES)

CXX = g++
INCLUDE_DIRS := include $(BUILD_DIR)
CXXFLAGS = -O3 -std=c++20 -Wall -Werror -Wpedantic -Wno-restrict -Wno-dangling-pointer
CXXFLAGS += $(addprefix -I,$(INCLUDE_DIRS))
LDFLAGS += $(shell pkg-config --libs yaml-cpp)

default: yavl-compiler

all: test

test: yavl-compiler
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


clean:
	$(RM) -r $(BUILD_DIR)

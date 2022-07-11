.PHONY: default all clean test

BUILD_DIR := build
SAMPLE_VALIDATOR_SOURCES := src/yavl-validate-sample.cpp src/yavl.cpp
SPEC_VALIDATOR_SOURCES := src/yavl-validate-spec.cpp src/yavl.cpp

CXX = g++
CXXFLAGS = -O3 -std=c++20 -Wall -Werror -Wpedantic -I./include
LDFLAGS += $(shell pkg-config --libs yaml-cpp)

default: yavl-validate-sample yavl-validate-spec

all: test

test: default
	./test.sh

yavl-validate-sample: $(BUILD_DIR)/yavl-validate-sample
	ln -sf $^ $@
	
$(BUILD_DIR)/yavl-validate-sample: $(addprefix $(BUILD_DIR)/,$(SAMPLE_VALIDATOR_SOURCES:.cpp=.o))
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -ldl -o $@

yavl-validate-spec: $(BUILD_DIR)/yavl-validate-spec
	ln -sf $^ $@

$(BUILD_DIR)/yavl-validate-spec: $(addprefix $(BUILD_DIR)/,$(SPEC_VALIDATOR_SOURCES:.cpp=.o))
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@	
	
$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $^

clean:
	$(RM) -r $(BUILD_DIR) yavl-validate-sample yavl-validate-spec

.PHONY: default all clean test

BUILD_DIR := build
COMPILER_SOURCES := src/yavl-compile.cpp src/yavl.cpp
SAMPLE_VALIDATOR_SOURCES := src/yavl-validate-sample.cpp src/yavl.cpp

CXX = g++
CXXFLAGS = -O3 -std=c++20 -Wall -Werror -Wpedantic -I./include
LDFLAGS += $(shell pkg-config --libs yaml-cpp)

default: yavl-compile yavl-validate-sample

all: test

test: default
	./test.sh

yavl-compile: $(BUILD_DIR)/yavl-compile
	ln -sf $^ $@

$(BUILD_DIR)/yavl-compile: $(addprefix $(BUILD_DIR)/,$(COMPILER_SOURCES:.cpp=.o))
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@

yavl-validate-sample: $(BUILD_DIR)/yavl-validate-sample
	ln -sf $^ $@
	
$(BUILD_DIR)/yavl-validate-sample: $(addprefix $(BUILD_DIR)/,$(SAMPLE_VALIDATOR_SOURCES:.cpp=.o))
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -ldl -o $@

$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $^

clean:
	$(RM) -r $(BUILD_DIR) yavl-validate-sample yavl-compile

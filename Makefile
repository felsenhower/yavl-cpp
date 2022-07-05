.PHONY: default all clean

BUILD_DIR := build
COMPILER_SOURCES := src/yavl-compiler.cpp src/yatc.cpp
VALIDATOR_SOURCES := src/yavl-validator.cpp src/yatc.cpp

CXX = g++
CXXFLAGS = -O3 -std=c++20 -Wall -Werror -Wpedantic -I./include
LDFLAGS += $(shell pkg-config --libs yaml-cpp)

default: yavl-compiler yavl-validator

all: test

yavl-compiler: $(BUILD_DIR)/yavl-compiler
	ln -sf $^ $@

$(BUILD_DIR)/yavl-compiler: $(addprefix $(BUILD_DIR)/,$(COMPILER_SOURCES:.cpp=.o))
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@

yavl-validator: $(BUILD_DIR)/yavl-validator
	ln -sf $^ $@
	
$(BUILD_DIR)/yavl-validator: $(addprefix $(BUILD_DIR)/,$(VALIDATOR_SOURCES:.cpp=.o))
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -ldl -o $@

$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $^

clean:
	$(RM) -r $(BUILD_DIR) yavl-validator yavl-compiler

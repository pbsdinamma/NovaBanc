CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude -Ithird_party
LDFLAGS  := 

DEBUG_FLAGS   := -g -O0
RELEASE_FLAGS := -O2

SRC_FILES  := $(wildcard src/*.cpp)
TEST_FILES := $(wildcard tests/*.cpp) $(filter-out src/main.cpp, $(SRC_FILES))

.PHONY: all run test debug clean help

all: build/novabanc

build/novabanc: $(SRC_FILES)
	-mkdir build data logs reports 2>nul
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) $^ -o $@ $(LDFLAGS)

debug: $(SRC_FILES)
	-mkdir build data logs reports 2>nul
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $^ -o build/novabanc_debug $(LDFLAGS)

run: all
	./build/novabanc

test:
	-mkdir build data logs reports 2>nul
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) $(TEST_FILES) -o build/run_tests $(LDFLAGS)
	./build/run_tests

clean:
	rm -rf build/ logs/*.log reports/*.csv

help:
	@echo "NovaBanc — Makefile targets:"
	@echo "  make          Build release binary (build/novabanc)"
	@echo "  make debug    Build with AddressSanitizer (build/novabanc_debug)"
	@echo "  make run      Build and run"
	@echo "  make test     Build and run all Catch2 tests"
	@echo "  make clean    Remove build artifacts and logs"

# Standalone unit tests for SWIM's OMNeT++-independent model classes.
# Requires only a C++ compiler -- no OMNeT++, Boost, or external test framework.
#
#   make test    # build and run
#   make clean

CXX      ?= g++
CXXFLAGS ?= -std=c++14 -Wall -Wextra -g

SRCROOT  := ../src
# -I$(SRCROOT)                -> "model/Configuration.h" style includes
# -I$(SRCROOT)/model/pladaptMock -> <pladapt/Configuration.h> mock headers
INCLUDES := -I$(SRCROOT) -I$(SRCROOT)/model/pladaptMock

MODEL_SRCS := \
	$(SRCROOT)/model/Configuration.cc \
	$(SRCROOT)/model/Environment.cc \
	$(SRCROOT)/model/Observations.cc

TEST_SRCS := main.cpp model_tests.cpp
BIN       := run_tests

$(BIN): $(TEST_SRCS) $(MODEL_SRCS) test_framework.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(TEST_SRCS) $(MODEL_SRCS)

.PHONY: test
test: $(BIN)
	./$(BIN)

.PHONY: clean
clean:
	rm -f $(BIN)

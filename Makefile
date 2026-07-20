CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -Iinclude

SRCS = src/protocol.cpp \
       src/messages.cpp \
       src/transport.cpp \
       src/router.cpp \
       src/device.cpp

OBJS = $(SRCS:.cpp=.o)

CLI_TARGET = oppoctl-cpp
TEST_TARGET = test_cpp_protocol

all: $(CLI_TARGET) $(TEST_TARGET)

$(CLI_TARGET): src/main_cli.o $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_TARGET): tests/test_cpp_protocol.o src/protocol.o src/messages.o
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	python3 generate_fixtures.py
	./$(TEST_TARGET)

clean:
	rm -f src/*.o tests/*.o $(CLI_TARGET) $(TEST_TARGET)

.PHONY: all test clean

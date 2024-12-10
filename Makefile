CXX = g++
CXXFLAGS = -std=c++20 -Include -g -Wall -Werror -pedantic -I $$(pwd)/include

SRC_DIR = src


# Define sources and objects
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
SRCS_TEST = src/application.cpp src/reliable_comm.cpp src/channels.cpp src/message.cpp src/atomic_broadcast_ring.cpp

OBJS = $(SRCS:.cpp=.o)
OBJS_TEST = $(SRCS_TEST:.cpp=.o)


# Define the executable
TARGET = my_project
TEST_TARGET = run_tests

# Define log files
LOGS = log*.txt

all: clean $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Build the test executable
$(TEST_TARGET): $(OBJS_TEST)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) tests/test_main.cpp $(OBJS_TEST)

# Rule for compiling .cpp to .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run test cases
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET) $(TEST_TARGET) $(LOGS)

CXX = g++
CXXFLAGS = -std=c++20 -Include -g -Wall -Werror -pedantic -I $$(pwd)/include

SRC_DIR = src

# Define sources and objects
# SRCS = src/application.cpp src/reliable_comm.cpp src/channels.cpp src/main.cpp src/message.cpp src/atomic_broadcast_ring.cpp
# SRCS_TEST = src/application.cpp src/reliable_comm.cpp src/channels.cpp src/message.cpp src/atomic_broadcast_ring.cpp
SRCS = $(wildcard $(SRC_DIR)/*.cpp)


OBJS = $(SRCS:.cpp=.o)
OBJS_TEST = $(SRCS_TEST:.cpp=.o)


# Define the executable
TARGET = my_project
TEST_TARGET = run_tests

# Define log files
LOGS = log*.txt *.log

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


# # Compiler and flags
# CXX = g++
# CXXFLAGS = -std=c++20 -Wall -pthread
# LDFLAGS = -pthread

# # Directories
# SRC_DIR = src
# INC_DIR = include
# OBJ_DIR = obj

# # Source files and object files
# SOURCES = $(wildcard $(SRC_DIR)/*.cpp) main.cpp
# OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# # Output binary
# TARGET = replicated_service

# # Default target to build everything
# all: $(TARGET)

# # Rule to create the output binary
# $(TARGET): $(OBJECTS)
# 	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# # Rule to compile the source files into object files
# $(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
# 	@mkdir -p $(OBJ_DIR)
# 	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -I../include -c $< -o $@

# # Rule to clean the build
# clean:
# 	rm -rf $(OBJ_DIR) $(TARGET)

# # Rule to run the application
# run: $(TARGET)
# 	./$(TARGET)

# # Phony targets
# .PHONY: all clean run

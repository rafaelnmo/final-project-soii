CXX = g++
CXXFLAGS = -std=c++20 -Include -g -Wall -Werror -pedantic -I $$(pwd)/include
CXXFLAGS = -std=c++20 -Include -g -Wall -Werror -pedantic -I $$(pwd)/include

# Define sources and objects
SRCS = src/application.cpp src/reliable_comm.cpp src/channels.cpp src/main.cpp src/message.cpp src/atomic_broadcast_ring.cpp
#SRCS := $(shell find src -name '*.cpp') src/message.cpp
OBJS = $(SRCS:.cpp=.o)

# Define the executable
TARGET = my_project

# Define log files
LOGS = log*.txt

all: clean $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rule for compiling .cpp to .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET) $(LOGS)

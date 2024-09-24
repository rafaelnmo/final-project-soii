CXX = g++
CXXFLAGS = -std=c++20 -Include -g -Wall -Werror -pedantic -I $$(pwd)/include

# Define sources and objects
SRCS = src/application.cpp src/reliable_comm.cpp src/failure_detection.cpp src/channels.cpp src/observer.cpp src/main.cpp src/message.cpp
#SRCS := $(shell find src -name '*.cpp')
OBJS = $(SRCS:.cpp=.o)

# Define the executable
TARGET = my_project

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rule for compiling .cpp to .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

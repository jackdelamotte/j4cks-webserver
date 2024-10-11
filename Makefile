# Makefile for building the cppserver

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -pthread

# Libraries to link
LIBS = -lboost_system -lboost_thread

# Source file
SRCS = server.cpp

# Output binary
TARGET = cppserver

# Build target
build:
	$(CXX) -o $(TARGET) $(SRCS) $(LIBS) $(CXXFLAGS)

# Clean target (optional for cleaning up the compiled binary)
clean:
	rm -f $(TARGET)

# Kill running cpp server
kill:
	sudo pkill cppserver

# Run the server
run:
	sudo ./cppserver

# Run in the background with nohup
run-bg:
	sudo nohup ./cppserver &


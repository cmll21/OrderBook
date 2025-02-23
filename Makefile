# Compiler
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -O2

# Source files
SRC = main.cpp order.cpp trade.cpp order_book.cpp matching_engine.cpp
OBJ = $(SRC:.cpp=.o)

# Output executable
TARGET = exe

# Default target: Compile the project
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

# Compile .cpp files into .o object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJ) $(TARGET)

# Run the compiled program
run: $(TARGET)
	./$(TARGET)

# Phony targets (not actual files)
.PHONY: all clean run

# Makefile

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -O2

# Source files and output directories
SRC = main.cpp  # Replace with the actual client filename
OUT_DIR = ./dist
OUT = $(OUT_DIR)/client  # Output executable name

# Default rule to build the client
all: $(OUT_DIR) $(OUT)

# Create output directory if it doesn't exist
$(OUT_DIR):
	mkdir -p $(OUT_DIR)

# Compile the client
$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(OUT) $(SRC)

# Clean up the build
clean:
	rm -rf $(OUT_DIR)

.PHONY: all clean

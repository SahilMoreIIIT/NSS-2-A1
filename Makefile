# Makefile for ACL Project using clang

# Compiler and flags
CC = clang
CFLAGS = -Wall -Werror -Iinclude

# Directories
SRC_DIR = src
BIN_DIR = bin

# Find all C source files in SRC_DIR
SRCS := $(wildcard $(SRC_DIR)/*.c)

# Map source files to executables in BIN_DIR (e.g., src/fget.c -> bin/fget)
BINS := $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(SRCS))

# Default target: build all executables
all: $(BIN_DIR) $(BINS)

# Create the bin directory if it doesn't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compile each source file into an executable
$(BIN_DIR)/%: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Clean target: remove all executables in bin/
clean:
	rm -f $(BIN_DIR)/*

.PHONY: all clean

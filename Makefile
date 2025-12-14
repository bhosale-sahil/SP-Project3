# Makefile for Multi-threaded Download Manager
# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
LDFLAGS = -lcurl -lpthread

# Target executable
TARGET = download_manager

# Source files
SOURCES = main.c network.c threads.c
OBJECTS = $(SOURCES:.c=.o)
HEADER = download_manager.h

# Default target
all: $(TARGET)
	@echo "Build complete! Run with: ./$(TARGET) -u <URL> -o <file> -t <threads>"

# Link object files to create executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

# Compile source files to object files
%.o: %.c $(HEADER)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build files..."
	rm -f $(TARGET) $(OBJECTS) *.dat *.mp4 *.iso
	@echo "Clean complete!"

# Quick test with 10MB file
test: $(TARGET)
	@echo "Running test download (10MB file)..."
	./$(TARGET) -u "https://proof.ovh.net/files/10Mb.dat" -o test.dat -t 4

# Test with 100MB file
test100: $(TARGET)
	@echo "Running test download (100MB file)..."
	./$(TARGET) -u "https://proof.ovh.net/files/100Mb.dat" -o test100.dat -t 8

# Test with 5400MB file
testiso: $(TARGET)
	@echo "Running test download (5400MB file)..."
	./$(TARGET) -u "./download_manager -u https://releases.ubuntu.com/25.10/ubuntu-25.10-desktop-amd64.iso" -o ubantu.iso -t 16

# Help
help:
	@echo "Multi-threaded Download Manager - Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  make          - Build the project (default)"
	@echo "  make all      - Build the project"
	@echo "  make clean    - Remove build files and downloads"
	@echo "  make test     - Build and run test with 10MB file"
	@echo "  make test100  - Build and run test with 100MB file"
	@echo "  make help     - Show this help message"
	@echo ""
	@echo "Usage after building:"
	@echo "  ./download_manager -u <URL> -o <output> -t <threads>"
	@echo ""
	@echo "Example:"
	@echo "  ./download_manager -u 'https://proof.ovh.net/files/10Mb.dat' -o file.dat -t 4"

.PHONY: all clean test test100 help

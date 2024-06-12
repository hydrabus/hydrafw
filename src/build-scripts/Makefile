# Compiler
CC = gcc
# Compiler flags
CFLAGS = -Wall -Wextra -std=c99

# Source files
SRCS = hex2dfu.c

# Output executable
TARGET = hex2dfu

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# Clean up
clean:
	rm -f $(TARGET)

# Phony targets
.PHONY: all clean
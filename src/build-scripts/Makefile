# Compiler
CC = gcc
# Compiler flags
CFLAGS = -Wall -Wextra -std=c99

# Source files
SRCS = dfu-convert.c

# Output executable
TARGET = dfu-convert

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
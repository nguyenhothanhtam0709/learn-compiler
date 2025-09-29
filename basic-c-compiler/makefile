CC = clang
CFLAGS = -std=c17 -Wall -Wextra -O2

# Project settings
TARGET = main
SRC    = $(wildcard src/*.c)
OBJ    = $(SRC:src/.c=build/.o)

# Default rule
all: $(TARGET)

# Link objects into final binary
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJ) $(TARGET)
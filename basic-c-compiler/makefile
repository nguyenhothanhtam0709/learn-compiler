CC = clang
CFLAGS = -std=c17 -Wall -Wextra -O2 -lm

# Project settings
X86_64_TARGET	= x86_64
AARCH64_TARGET	= aarch64
SRC    			= $(wildcard src/*.c)
SRC    			:= $(filter-out src/cg_arm.c src/expr2.c src/interp.c, $(SRC))
X86_64_SRC 		:= $(filter-out src/cg_aarch64.c, $(SRC))
AARCH64_SRC 	:= $(filter-out src/cg.c, $(SRC))
X86_64_OBJ    	= $(X86_64_SRC:src/.c=build/.o)
AARCH64_OBJ    	= $(AARCH64_SRC:src/.c=build/.o)

test_x86_64: $(X86_64_TARGET) tests/runtests
	(cd tests; chmod +x runtests; ./runtests x86_64)

test_aarch64: $(AARCH64_TARGET) tests/runtests
	(cd tests; chmod +x runtests; ./runtests aarch64)

# Link objects into final binary
$(X86_64_TARGET): $(X86_64_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(AARCH64_TARGET): $(AARCH64_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(X86_64_TARGET) $(AARCH64_TARGET)
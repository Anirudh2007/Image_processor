# ============================================================
#  Makefile for bmp-parser
#  Targets: all (default), clean, run
# ============================================================

# --- Toolchain ---
CC      := gcc

# --- Flags ---
# -Wall -Wextra  : Enable broad and extended warnings.
# -std=c11       : C11 is required because we use _Static_assert, which was
#                  introduced in C11. C99 does not have it.
# -pedantic      : Reject any code that is not strictly ISO-compliant.
# -Iinclude      : Add /include to the header search path so source files
#                  can use #include <bmp_core.h> without relative ../paths.
CFLAGS  := -Wall -Wextra -std=c11 -pedantic -Iinclude

# --- Directories ---
SRC_DIR := src
OBJ_DIR := obj

# --- Target executable ---
TARGET  := bmp_tool

# --- Source and Object files ---
# Automatically find all .c files under /src so you never need to
# manually update this list when adding new translation units.
SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# ============================================================
#  Default target: build the executable
# ============================================================
all: $(OBJ_DIR) $(TARGET)

# Link all object files into the final binary.
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo "Build successful -> $(TARGET)"

# Compile each .c file into a .o object file inside /obj.
# The $< is the first prerequisite (.c file), $@ is the target (.o file).
# Separating compilation from linking lets make rebuild only changed files.
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Create the object directory if it does not exist.
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# ============================================================
#  clean: remove all build artefacts
# ============================================================
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "Cleaned build artefacts."

# ============================================================
#  run: quick smoke-test shortcut (override ARGS on the CLI)
#  Usage: make run ARGS="info test_images/sample.bmp"
# ============================================================
run: all
	./$(TARGET) $(ARGS)

# Declare targets that are not real files so make doesn't get
# confused if a file named 'clean' or 'run' exists in the tree.
.PHONY: all clean run

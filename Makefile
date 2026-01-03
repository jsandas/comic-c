# Makefile for Captain Comic C refactor
# Uses local Open Watcom 2 installation for building
# Ensure setvars.sh has been sourced before running make

# Source directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
REFERENCE_DIR = reference/disassembly

# Compiler and linker
WCC = wcc
WCFLAGS = -ml -s -0 -i=$(INCLUDE_DIR)
NASM = nasm
NASMFLAGS = -f obj
WLINK = wlink
WLINKFLAGS = system dos name $(EXECUTABLE) file

# Source files
C_SOURCES = $(wildcard $(SRC_DIR)/*.c)
C_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.obj,$(C_SOURCES))
ASM_SOURCE = $(SRC_DIR)/R5sw1991_c.asm
ASM_OBJECT = $(OBJ_DIR)/R5sw1991.obj

# Output executable
EXECUTABLE = $(BUILD_DIR)/COMIC-C.EXE

.PHONY: all compile clean shell help

# Default target
all: compile

# Compile the project
compile: $(EXECUTABLE)
	@echo "Build complete: $(EXECUTABLE)"

# Link executable
$(EXECUTABLE): $(C_OBJECTS) $(ASM_OBJECT)
	@echo "Linking $(EXECUTABLE)..."
	@echo "system dos" > $(BUILD_DIR)/comic.lnk
	@echo "option nodefaultlibs" >> $(BUILD_DIR)/comic.lnk
	@echo "name $(EXECUTABLE)" >> $(BUILD_DIR)/comic.lnk
	@echo "file $(ASM_OBJECT)" >> $(BUILD_DIR)/comic.lnk
	@for obj in $(C_OBJECTS); do echo "file $$obj" >> $(BUILD_DIR)/comic.lnk; done
	$(WLINK) @$(BUILD_DIR)/comic.lnk

# Compile C sources
$(OBJ_DIR)/%.obj: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@mkdir -p $(OBJ_DIR)
	$(WCC) $(WCFLAGS) -fo=$@ $<

# Assemble assembly source
$(ASM_OBJECT): $(ASM_SOURCE)
	@echo "Assembling $<..."
	@mkdir -p $(OBJ_DIR)
	$(NASM) $(NASMFLAGS) -o $@ $<

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	@echo "Clean complete."

# Show help
help:
	@echo "Captain Comic C Refactor - Makefile Targets"
	@echo ""
	@echo "Usage: source setvars.sh && make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  make compile   - Compile the project using local Open Watcom 2"
	@echo "  make clean     - Remove all build artifacts"
	@echo "  make help      - Show this help message"
	@echo ""
	@echo "First-time setup:"
	@echo "  1. source setvars.sh"
	@echo "  2. make compile"
	@echo ""
	@echo "For development:"
	@echo "  source setvars.sh"
	@echo "  make compile   - Recompile after changes"

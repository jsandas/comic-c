# Makefile for Captain Comic C refactor
# Uses Docker to provide Open Watcom 2 build environment on macOS

# Docker image name
DOCKER_IMAGE = comic-c-build

# Docker container name
DOCKER_CONTAINER = comic-c-builder

# Workspace mount point
WORKSPACE = /workspace

# Source directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
REFERENCE_DIR = reference/disassembly

# Compiler and linker
WCC = wcc
WCFLAGS = -ml -s -0 -i=$(WORKSPACE)/$(INCLUDE_DIR)
NASM = nasm
NASMFLAGS = -f obj
DJLINK = $(WORKSPACE)/$(REFERENCE_DIR)/djlink/djlink
DJLINKFLAGS =

# Source files
C_SOURCES = $(wildcard $(SRC_DIR)/*.c)
C_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.obj,$(C_SOURCES))
ASM_SOURCE = $(REFERENCE_DIR)/R5sw1991_c.asm
ASM_OBJECT = $(OBJ_DIR)/R5sw1991.obj

# Output executable
EXECUTABLE = $(BUILD_DIR)/COMIC-C.EXE

.PHONY: all docker-build compile clean shell help

# Default target
all: compile

# Build Docker image
docker-build:
	@echo "Building Docker image: $(DOCKER_IMAGE)..."
	docker build -t $(DOCKER_IMAGE) .
	@echo "Docker image built successfully."

# Compile inside Docker container
compile:
	@echo "Compiling Captain Comic in Docker container..."
	@mkdir -p $(BUILD_DIR) $(OBJ_DIR)
	docker run --rm \
		-v "$(PWD):$(WORKSPACE)" \
		-w $(WORKSPACE) \
		$(DOCKER_IMAGE) \
		bash -c "make docker-compile"

# This target runs inside the Docker container
docker-compile: $(EXECUTABLE)
	@echo "Build complete: $(EXECUTABLE)"

# Link executable
$(EXECUTABLE): $(C_OBJECTS) $(ASM_OBJECT) $(DJLINK)
	@echo "Linking $(EXECUTABLE)..."
	$(DJLINK) -o $@ $(ASM_OBJECT) $(C_OBJECTS) $(DJLINKFLAGS)

# Build djlink linker if needed
$(DJLINK):
	@echo "Building djlink linker..."
	@cd $(REFERENCE_DIR)/djlink && \
		g++ -O2 -Wall -std=c++98 -o djlink \
		djlink.cc fixups.cc libs.cc list.cc map.cc objs.cc \
		out.cc quark.cc segments.cc stricmp.cc symbols.cc \
		2>&1 | grep -v "warning:" || true
	@chmod +x $(DJLINK) 2>/dev/null || true

# Compile C sources
$(OBJ_DIR)/%.obj: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@mkdir -p $(OBJ_DIR)
	$(WCC) $(WCFLAGS) -fo=$@ $<

# Assemble assembly source
$(ASM_OBJECT): $(ASM_SOURCE)
	@echo "Assembling $<..."
	@mkdir -p $(OBJ_DIR)
	cd $(REFERENCE_DIR) && $(NASM) $(NASMFLAGS) -o $(WORKSPACE)/$(OBJ_DIR)/R5sw1991.obj R5sw1991_c.asm

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	@echo "Clean complete."

# Open shell in Docker container for debugging
shell:
	@echo "Opening shell in Docker container..."
	docker run --rm -it \
		-v "$(PWD):$(WORKSPACE)" \
		-w $(WORKSPACE) \
		$(DOCKER_IMAGE) \
		bash

# Show help
help:
	@echo "Captain Comic C Refactor - Makefile Targets"
	@echo ""
	@echo "  make docker-build  - Build Docker image with Open Watcom 2, NASM, djlink"
	@echo "  make compile       - Compile the project inside Docker container"
	@echo "  make clean         - Remove all build artifacts"
	@echo "  make shell         - Open interactive shell in Docker container"
	@echo "  make help          - Show this help message"
	@echo ""
	@echo "First-time setup:"
	@echo "  1. make docker-build"
	@echo "  2. make compile"
	@echo ""
	@echo "For development:"
	@echo "  make compile       - Recompile after changes"

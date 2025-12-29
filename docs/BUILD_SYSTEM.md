# Build System Quick Reference

## Makefile Targets

| Target | Description |
|--------|-------------|
| `make docker-build` | Build Docker image with Open Watcom 2, NASM, djlink (run once) |
| `make compile` | Compile project inside Docker container (default target) |
| `make clean` | Remove all build artifacts (`build/` directory) |
| `make shell` | Open interactive shell inside Docker container for debugging |
| `make help` | Display help message with available targets |

## First-Time Setup

```bash
# 1. Build Docker image (only needed once, or when Dockerfile changes)
make docker-build

# 2. Compile the project
make compile

# 3. Executable will be at: build/COMIC.EXE
```

## Development Workflow

```bash
# Edit source files
vim src/game_main.c
vim src/R5sw1991_c.asm

# Recompile (fast - Docker image already built)
make compile

# Test in DOSBox
cd tests
./run-dosbox.sh

# If compilation errors, open shell for debugging
make shell
# Inside container:
#   wcc -ml -s -0 -i=include src/game_main.c
#   nasm -f obj src/R5sw1991_c.asm
```

## Build Process Details

### Compilation Steps

1. **Assemble** `src/R5sw1991_c.asm` → `build/obj/R5sw1991.obj`
   ```bash
   nasm -f obj -o build/obj/R5sw1991.obj src/R5sw1991_c.asm
   ```

2. **Compile** `src/*.c` → `build/obj/*.obj`
   ```bash
   wcc -ml -s -0 -i=include src/game_main.c -fo=build/obj/game_main.obj
   wcc -ml -s -0 -i=include src/file_loaders.c -fo=build/obj/file_loaders.obj
   ```

3. **Link** object files → `build/COMIC.EXE`
   ```bash
   djlink -o build/COMIC.EXE build/obj/R5sw1991.obj build/obj/*.obj
   ```

### Compiler Flags

- **`-ml`**: Large memory model (separate 64KB code/data segments)
- **`-s`**: No stack overflow checks (DOS real-mode doesn't need them)
- **`-0`**: Target 8086 CPU (maximum compatibility)
- **`-i=include`**: Include directory for header files
- **`-fo=<file>`**: Output object file path

### Assembler Flags

- **`-f obj`**: Output OMF object format (compatible with djlink)
- **`-o <file>`**: Output file path

## Docker Container Details

### Image Contents
- **Base**: Debian Bullseye slim
- **Open Watcom 2**: C compiler for 16-bit DOS
  - Location: `/opt/watcom/`
  - Binary: `/opt/watcom/binl64/wcc` (16-bit compiler)
- **NASM**: x86 assembler
  - Installed via apt: `/usr/bin/nasm`
- **djlink**: OMF linker
  - Mounted from workspace: `reference/disassembly/djlink/djlink`

### Volume Mount
- **Host**: `$(PWD)` (current directory on macOS)
- **Container**: `/workspace`
- **Working Dir**: `/workspace`

All file paths in Makefile are relative to `/workspace` inside the container.

### Environment Variables (in container)
```bash
WATCOM=/opt/watcom
PATH=$WATCOM/binl64:$WATCOM/binl:$PATH
INCLUDE=$WATCOM/h
```

## Troubleshooting

### "Docker image not found"
```bash
# Solution: Build the image
make docker-build
```

### "wcc: command not found" (inside container)
```bash
# Check Open Watcom installation
ls -la /opt/watcom/binl64/wcc
echo $WATCOM
echo $PATH

# Rebuild Docker image if missing
exit  # Exit container
make docker-build
```

### "djlink: Permission denied"
```bash
# djlink needs execute permissions
chmod +x reference/disassembly/djlink/djlink
make compile
```

### "nasm: No such file or directory"
```bash
# NASM should be installed in Docker image
# Rebuild image if missing
make docker-build
```

### Compilation errors in C code
```bash
# Open shell to debug
make shell

# Inside container, compile individual files to see detailed errors
cd /workspace
wcc -ml -s -0 -i=include src/game_main.c -fo=build/obj/game_main.obj

# Check header files
ls -la include/
cat include/globals.h
```

### Linker errors
```bash
# Check that all object files exist
ls -la build/obj/

# Manually link to see detailed errors
make shell
djlink -o build/COMIC.EXE build/obj/R5sw1991.obj build/obj/game_main.obj

# Common issue: undefined symbol
# - Check that assembly exports with `global` directive
# - Check that C declares with `#pragma aux "*"` and `extern`
```

## Clean Build

```bash
# Remove all build artifacts
make clean

# Full rebuild
make docker-build  # Only if Dockerfile changed
make compile
```

## Manual Compilation (without Docker)

If you have Open Watcom 2 installed natively (Linux only):

```bash
# Set up environment
export WATCOM=/path/to/watcom
export PATH=$WATCOM/binl64:$PATH

# Compile
mkdir -p build/obj
nasm -f obj -o build/obj/R5sw1991.obj src/R5sw1991_c.asm
wcc -ml -s -0 -i=include src/game_main.c -fo=build/obj/game_main.obj
wcc -ml -s -0 -i=include src/file_loaders.c -fo=build/obj/file_loaders.obj
reference/disassembly/djlink/djlink -o build/COMIC.EXE build/obj/*.obj
```

**Note**: Open Watcom 2 is not available for macOS, hence the Docker approach.

## File Sizes Reference

- **Original COMIC.EXE**: ~33 KB
- **Current build** (with C code): ~100 KB
  - Larger due to C code and Open Watcom library functions
  - Size will vary as more assembly is converted to C
  - Final size expected to be 80-120 KB

## Performance Notes

- **Docker overhead**: First `make compile` starts container (~1-2 seconds)
- **Compilation speed**: Fast - seconds for full rebuild
- **Incremental builds**: Only recompile changed files (Makefile handles dependencies)
- **Docker caching**: Image is cached, `docker-build` only needed once

## Integration with IDE

### VS Code
```json
// .vscode/tasks.json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Build Captain Comic",
      "type": "shell",
      "command": "make compile",
      "group": {
        "kind": "build",
        "isDefault": true
      }
    }
  ]
}
```

### Command Line
```bash
# Quick rebuild and test
make compile && cd tests && ./run-dosbox.sh
```

# Build System Quick Reference

## Makefile Targets

| Target | Description |
|--------|-------------|
| `make compile` | Compile project using local Open Watcom (default target) |
| `make clean` | Remove all build artifacts (`build/` directory) |
| `make help` | Display help message with available targets |

## First-Time Setup

```bash
# 1. Set up Open Watcom environment (required every session)
source setvars.sh

# 2. Compile the project
make compile

# 3. Executable will be at: build/COMIC-C.EXE
```

## Development Workflow

```bash
# Set up environment (once per terminal session)
source setvars.sh

# Edit C source files (main entry point)
vim src/game_main.c

# Recompile (fast)
make compile

# Test in DOSBox
cd tests
./run-dosbox.sh

# If compilation errors, compile manually for detailed output:
wcc -ml -s -0 -i=include src/game_main.c -fo=build/obj/game_main.obj
wcc -ml -s -0 -i=include src/file_loaders.c -fo=build/obj/file_loaders.obj
nasm -f obj -o build/obj/R5sw1991.obj src/R5sw1991_c.asm
djlink -o build/COMIC-C.EXE build/obj/*.obj
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

## Local Build Environment

### Installation
- **Open Watcom 2**: Located in `watcom2/` directory
  - Binary: `watcom2/bino64/wcc` (16-bit compiler)
- **NASM**: x86 assembler (install via Homebrew: `brew install nasm`)
- **djlink**: OMF linker
  - Located in `reference/disassembly/djlink/djlink`
  - Built for macOS using clang++

### Environment Setup

The `setvars.sh` script configures the build environment:

```bash
export WATCOM=$(pwd)/watcom2
export PATH=$WATCOM/bino64:$PATH
export EDPATH=$WATCOM/eddat
export WIPFC=$WATCOM/wipfc
export INCLUDE=$WATCOM/h
export LIB=$WATCOM/lib286
```

**Important**: Run `source setvars.sh` before compiling in each terminal session.

## Troubleshooting

### "wcc: command not found"
```bash
# Solution: Source the environment setup
source setvars.sh

# Verify environment
echo $WATCOM
echo $PATH
which wcc
```

### "djlink: cannot execute binary file"
```bash
# Solution: Rebuild djlink for macOS
rm -f reference/disassembly/djlink/djlink
make compile  # Makefile will rebuild djlink using clang++
```

### "nasm: command not found"
```bash
# Solution: Install NASM via Homebrew
brew install nasm

# Verify installation
which nasm
nasm -v
```

### "Permission denied" for djlink
```bash
# djlink needs execute permissions
chmod +x reference/disassembly/djlink/djlink
make compile
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
source setvars.sh
make compile
```

## Manual Compilation

If you need to compile individual files for debugging:

```bash
# Set up environment
source setvars.sh

# Create build directories
mkdir -p build/obj

# Assemble
nasm -f obj -o build/obj/R5sw1991.obj src/R5sw1991_c.asm

# Compile C files
wcc -ml -s -0 -i=include src/game_main.c -fo=build/obj/game_main.obj
wcc -ml -s -0 -i=include src/file_loaders.c -fo=build/obj/file_loaders.obj
wcc -ml -s -0 -i=include src/rendering.c -fo=build/obj/rendering.obj
wcc -ml -s -0 -i=include src/runtime_stubs.c -fo=build/obj/runtime_stubs.obj

# Link
reference/disassembly/djlink/djlink -o build/COMIC-C.EXE \
  build/obj/R5sw1991.obj \
  build/obj/file_loaders.obj \
  build/obj/game_main.obj \
  build/obj/rendering.obj \
  build/obj/runtime_stubs.obj
```

## File Sizes Reference

- **Original COMIC.EXE**: ~33 KB
- **Current build** (with C code): ~100 KB
  - Larger due to C code and Open Watcom library functions
  - Size will vary as more assembly is converted to C
  - Final size expected to be 80-120 KB

## Performance Notes

- **Compilation speed**: Very fast - seconds for full rebuild
- **Incremental builds**: Only recompile changed files (Makefile handles dependencies)
- **Native performance**: No containerization overhead
- **djlink compilation**: First build compiles djlink with clang++ (~1-2 seconds)

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

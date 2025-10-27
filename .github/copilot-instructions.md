# Copilot Instructions for Stegano

## Project Overview
This is a C program for image steganography, focused on encoding and decoding messages within BMP image files. The codebase is structured for educational purposes in fundamental C programming.

## Key Components
- `main.c`: Entry point, CLI argument processing, and high-level workflow.
- `stegano.h` / `stegano.c`: Core logic for message compression/decompression (RLE), queue management, and utility functions.
- `README.md`: Brief project description.
- `makefile`: (Currently empty) — build commands should be added here for automation.

## Architecture & Data Flow
- **Encoding/Decoding**: Functions `encode` and `decode` operate on BMP files, handling message embedding and extraction.
- **Compression**: Messages are compressed (likely using RLE) before embedding, via `compressMessage` and `decompressMessage`.
- **Queue Structure**: Recent files are tracked using a custom queue (`queue_t`), with operations defined in `stegano.h`.

## Developer Workflows
- **Build**: No build commands are present. Add rules to `makefile` for compiling, e.g.:
  ```makefile
  stegano: main.c stegano.c stegano.h
  	gcc -o stegano main.c stegano.c
  ```
  Then run: `make stegano`
- **Run**: Execute the binary with appropriate arguments for encoding/decoding.
- **Debug**: Use standard C debugging tools (e.g., `gdb`).

## Project-Specific Patterns
- **BMP Handling**: Always validate file type with `checkFileType` before processing.
- **Compression**: Use RLE for message compression/decompression.
- **Queue Usage**: Track recent files via queue operations (`enqueue`, `dequeue`, etc.).
- **Function Ownership**: Comments indicate which developer is responsible for each function (e.g., Khanh, Sam).

## Integration Points
- No external libraries or dependencies are used; all logic is implemented in C.
- The program expects BMP files as input/output.

## Conventions
- All major functions are declared in `main.c` and implemented in `stegano.c`.
- Queue structure is defined in `stegano.h` and implemented in `stegano.c`.
- Use clear comments to indicate function purpose and ownership.

## Example Usage
```sh
./stegano encode input.bmp output.bmp "Secret message"
./stegano decode input.bmp
```

## Recommendations for AI Agents
- When adding features, follow the compression and queue patterns in `stegano.c`/`stegano.h`.
- Update the `makefile` to automate builds as needed.
- Maintain clear function documentation and ownership comments.
- Validate BMP files before processing.

---
If any section is unclear or missing, please provide feedback for further refinement.

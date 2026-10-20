# PXE Emulator (C++)

Rebuilt C++ PXE Emulator inspired by and based on the LG PXE Web Emulator by **@tova98** ([https://pxe.nefal.world/](https://pxe.nefal.world/)).

## Features

- **ARMv5TE CPU Core**: ARM and Thumb instruction interpreter with 64MB flat memory, CPSR flags, and HLE trap support.
- **PXO Game Loader**: Support for LG `.pxo` games, including LGLZ stream decompression and PXO header parsing (`roSize`, `rwBase`, `rwEnd`, `ziBase`, `ziEnd`, `init`, `event`).
- **FLS Kernel & Executable Loader**: Placeholder system (`FLSLoader`) to load real kernels and executables in `.fls` format (FLS container format headers, block section tables, memory mapping, and raw image fallback).
- **LG PXE HLE API Engine**: Memory management (`malloc`/`free`), system calls, screen canvas, and touch event handling.
- **Win32 Application GUI**: Integrated GUI with File -> Open `.pxo` and File -> Open `.fls` menu options, rendering canvas, and touch input support.

## Credits

Special thanks and full credit to **@tova98** for creating the original PXE emulator architecture at [https://pxe.nefal.world/](https://pxe.nefal.world/).

## Building

### Visual Studio (Windows)

Open `PXE Emulator.sln` in Visual Studio 2022 and build the project (`PXE Emulator.vcxproj`).

### CMake / GCC / Clang (Cross-Platform / Linux / Windows)

```bash
mkdir build
cd build
cmake ..
make
./test_pxe
```

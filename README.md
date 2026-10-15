# enigma

Short description of the project

## Installation

## Building from source

Make sure that you have [ZDE](https://github.com/zoul0813/zeal-dev-environment) installed.

From the project directory, build with:

```shell
zde cmake
```

This uses the ZDE SDCC toolchain and the ZVB SDK, ZGDK, and ZAR CMake packages.
The build converts the checked-in GIFs to 4-bit graphics, exports the world as
64 compressed 40×21 rooms, and generates archive indices in `build/include/enigma_assets.h`.
Generated assets and intermediate files go in `build/`.

Copy both `bin/enigma.bin` and `bin/enigma.zar` to the same directory on Zeal.
Debug artifacts go in `debug/`.

The legacy `Makefile` remains available through `zde make`.

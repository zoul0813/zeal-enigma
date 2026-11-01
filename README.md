# Enigma using world.lib

This working clone uses zeal-world-engine's `lib/world.lib` for room navigation,
world position, logical tilemap registration, and room loading coordination.
`src/game_world.c` configures an 8x8 world with 40x21-tile rooms at tile offset
(0,4), leaving space for the HUD. Transitions are immediate; the optional slide
and minimap modules are not linked into this game.

Enigma owns its archive/decompression, tileset and palette uploads, room object
overlays, player movement and arrival positions, pickups, gates, and enemies.
The game-side outer-world boundary clamp now uses the room's pixel bounds;
the original vertical clamp mixed tile offsets with pixel coordinates.

## Build

With [ZDE](https://github.com/zoul0813/zeal-dev-environment) installed, run from
the parent zeal-world-engine repository:

```sh
zde --no-tty exec cmake -S /src -B /src/build
zde --no-tty exec cmake --build /src/build --target world_engine
zde --no-tty exec cmake -S /src/tmp/zeal-enigma -B /src/tmp/zeal-enigma/build
zde --no-tty exec cmake --build /src/tmp/zeal-enigma/build
```

The clone defaults to the engine two directories above it. For another layout,
configure with `-DWORLD_ENGINE_PATH=/path/to/zeal-world-engine` using the path
visible inside ZDE. Build the engine library first; Enigma links the prebuilt
archive through the usual SDCC library options. Outputs are `bin/enigma`
and `bin/enigma.zar`. CMake is the supported build; the legacy Makefile has been removed upstream.

## Run

From the parent repository:

```sh
zeal-native -H tmp/zeal-enigma/bin
```

At the Zeal OS prompt, enter `cd H:/`, then `./enigma`.

## Verify

From this clone:

```sh
python3 tests/verify_world.py --baseline build/baseline
python3 tests/verify_world.py
```

The baseline artifacts are a fresh build of upstream commit `ffc3a81`, before
the engine migration, and are local build outputs. Both commands use the installed `zeal-native` debugger and
compare startup state and all 840 logical room tiles against the original game
and TMX data. Checks cover shooting, enemy setup, key/gate and fuel behavior,
all four transition directions, arrival positions, persistent pickups, and
world boundary clamping (the latter on the migrated build only).

These are focused emulator checks that place the player near objects and room
edges, rather than a full playthrough. Upstream gameplay and asset-loader fixes are retained.

The verifier reads ROM images from `~/.zeal8bit/roms/` by default; override with
`ZEAL_NATIVE_ROM`, `ZEAL_NATIVE_EEPROM`, and `ZEAL_NATIVE_TF` if needed. Writable
disk images are copied to a temporary directory before running.

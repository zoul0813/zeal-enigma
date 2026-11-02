# Enigma using world.lib

This working clone uses zeal-world-engine's `lib/world.lib` for room navigation,
world position, logical tilemap registration, and room loading coordination.
`src/game_world.c` configures an 8x8 world with 40x21-tile rooms at tile offset
(0,4), leaving space for the HUD. Transitions are immediate; the optional slide
and minimap modules are not linked into this game.

Enigma owns its archive/decompression, tileset and palette uploads, room object
overlays, player movement and arrival positions, pickups, gates, and enemies.
The game-side outer-world boundary clamp now uses the room's pixel bounds;
the earlier Zeal port mixed tile offsets with pixel coordinates.

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

## Original-game behavior

Enigma is based on the 1985 maze shooter by Mark J. Moore and Peter J. Bartlett,
published by CBM UK.

The port now uses the original per-room enemy population/speed tables and four
difficulty levels. All active enemies pursue together on alternate 50 Hz logic
ticks after a difficulty-dependent pause. Eight enemy appearances award the
original 10–800 points; keys award 5,000. Rooms repopulate after re-entry.

Arrows select difficulty on the title screen; Space starts. During play, arrows
move in eight directions and Space fires repeatedly while moving. The laser has
215 units and eight projectile slots; energy cells refill it. Enemy contact
costs one of three lives. Respawning restores the room-entry position and laser
energy while retaining keys, opened gates, collected fuel, and score. After the
last life, Space starts a fresh game; the high score is retained until exit.
Right Shift or Quote exits through the SDK Select mapping.

The larger Zeal sprites use adapted collision footprints and movement scaling.
The death presentation is a short pause, rather than the original palette and
sound sequence. Original sound, exact animation, and final-room message-letter
pickups remain unfinished. The original has no formal victory screen, so this
port does not introduce one.

## Verify

From this clone:

```sh
python3 tests/test_assets.py
python3 tests/test_gameplay.py
python3 tests/verify_world.py
python3 tests/verify_gameplay.py
```

Host tests use ASan/UBSan to check asset safety and gameplay rules. The target
checks use the installed `zeal-native` debugger with the freshly built binary
and link map. They cover room geometry and all four transitions, persistent
pickups/gates, difficulty selection, repeated fire, contact deaths, respawn,
game over/restart, and high score. They place actors near test cases rather
than performing a complete playthrough. `ZGDK_PATH` can select the SDK checkout
for host gameplay tests.

The emulator reads ROM images from `~/.zeal8bit/roms/` by default; override with
`ZEAL_NATIVE_ROM`, `ZEAL_NATIVE_EEPROM`, and `ZEAL_NATIVE_TF`. Writable disk
images are copied to a temporary directory before running. Logs are written to
`build/verify-world-lib.log` and `build/verify-gameplay.log`.

## Performance

Run `python3 tests/profile_gameplay.py` to measure emulated 10 MHz CPU timing
from the normal build without adding instrumentation. The profiler writes
`build/profile-gameplay.json` and its debugger transcript.

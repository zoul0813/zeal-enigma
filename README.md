# Enigma using ZGDK rooms

This game uses ZGDK room management in `zgdk.lib` for room navigation,
world position, logical tilemap registration, and room loading coordination.
`src/game_world.c` configures an 8x8 world with 40x21-tile rooms at tile offset
(0,4), leaving space for the HUD. Transitions are immediate; the optional slide
and minimap modules are not linked into this game.

Enigma owns its archive/decompression, tileset and palette uploads, room object
overlays, player movement and arrival positions, pickups, gates, and enemies.
The game-side outer-world boundary clamp now uses the room's pixel bounds;
the earlier Zeal port mixed tile offsets with pixel coordinates.

## Build

With [ZDE](https://github.com/zoul0813/zeal-dev-environment) and a ZGDK build
containing the world module, run from this checkout:

```sh
zde --no-tty cmake
```

No separate world-engine checkout, `world.lib` build, or `WORLD_ENGINE_PATH` is
required. CMake links only the SDK libraries. Outputs are `bin/enigma` and
`bin/enigma.zar`. CMake is the supported build.

For the sibling SDK clone, configure from `Projects/zeal` after building that
clone's `zgdk.lib`:

```sh
zde --no-tty exec cmake --fresh -S /src/zeal-world-engine/tmp/zeal-enigma -B /src/zeal-world-engine/tmp/zeal-enigma/build -DZGDK_DIR=/src/zeal-game-dev-kit/cmake
zde --no-tty exec cmake --build /src/zeal-world-engine/tmp/zeal-enigma/build --parallel 4
```

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
difficulty levels. Player logic runs at nominally 50 Hz, scheduled as five ticks
per six 60 Hz video frames. Input and rendering run every video frame, while
all enemies and all bullets run on opposite logic ticks (about 25 Hz each), after a
difficulty-dependent enemy entry pause. Eight enemy types award the original
10–800 points and share the five complete graphic variants supplied by the sprite
sheet. Keys award 5,000. Rooms repopulate after re-entry.

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
# Check debugger record offsets inside ZDE after building:
# zde --no-tty exec python3 /src/tmp/zeal-enigma/tests/target_layout.py
python3 tests/test_assets.py
python3 tests/test_gameplay.py
python3 tests/verify_world.py
python3 tests/verify_gameplay.py
python3 tests/verify_keyboard.py
# Sweep release timing every 50 CPU cycles across sprite uploads:
python3 tests/verify_keyboard.py --sweep --report build/verify-keyboard.json
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

At the measured 59.64 Hz video refresh, the 50/25 Hz schedule targets about
49.70 player updates and 24.85 enemy/projectile passes per second. The profiler
checks the five-in-six schedule and alternating actor passes under the controlled
16-enemy/eight-projectile workload. Room loading and spawning still cause entry stalls.

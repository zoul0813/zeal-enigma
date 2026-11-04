"""Shared debugger fixture offsets; run in ZDE to verify against SDCC's ABI."""
ENEMY_STRIDE = 9
ENEMY_TAG = 4
ENEMY_LEFT = 5
ENEMY_RIGHT = 7
BULLET_STRIDE = 5
BULLET_SPRITE = 2
BULLET_TAG = 4


def check():
    from pathlib import Path
    import re
    import shlex
    import shutil
    import subprocess

    root = Path(__file__).resolve().parents[1]
    flags = (root / "build/CMakeFiles/enigma.dir/flags.make").read_text()
    options = []
    for name in ("C_DEFINES", "C_INCLUDES", "C_FLAGS"):
        options += shlex.split(re.search(r"^" + name + r" = (.*)$", flags, re.M)[1])
    source = root / "build/target-layout.c"
    checks = {
        "sizeof(enemy_t)": ENEMY_STRIDE,
        "offsetof(enemy_t, point)": 0,
        "offsetof(enemy_t, type)": ENEMY_TAG,
        "offsetof(enemy_t, sprite_l)": ENEMY_LEFT,
        "offsetof(enemy_t, sprite_r)": ENEMY_RIGHT,
        "sizeof(bullet_t)": BULLET_STRIDE,
        "offsetof(bullet_t, sprite)": BULLET_SPRITE,
        "offsetof(bullet_t, type)": BULLET_TAG,
        "sizeof(entity_pool_t)": 7,
        "offsetof(entity_pool_t, used_end)": 2,
    }
    source.write_text('#include <stddef.h>\n#include "enemies.h"\n#include "bullet.h"\n' +
                      ''.join(f'_Static_assert({expr} == {size}, "debugger layout: {expr}");\n'
                              for expr, size in checks.items()))
    subprocess.run([shutil.which("sdcc") or "/opt/sdcc/bin/sdcc", *options,
                    "-I" + str(root / "src"), "-c", str(source),
                    "-o", str(root / "build/target-layout.rel")], check=True)
    print("PASS: SDCC enemy/projectile/pool layouts match debugger fixtures")


if __name__ == "__main__":
    check()

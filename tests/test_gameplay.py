"""Exercise real gameplay code on the host; hardware is replaced with a room map.

ZGDK_PATH can select a checkout of the SDK used by the target build. Only its
plain geometry/type definitions are reused; no target libraries execute here.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
sdk = Path(os.environ.get("ZGDK_PATH", str(root.parents[2] / "zeal-game-dev-kit")))
if not (sdk / "include/zgdk/types.h").is_file():
    raise SystemExit("Set ZGDK_PATH to the SDK checkout used by this game")

with tempfile.TemporaryDirectory(prefix="enigma-gameplay-") as directory:
    temporary = Path(directory)
    for name in (
        "zvb_gfx.h", "zvb_sprite.h", "zar.h", "zos_errors.h",
        "zgdk/tilemap.h", "zgdk/sprites.h", "zgdk/tiles.h",
        "zgdk/utils/print.h", "zgdk/utils/rand.h", "world/world.h",
    ):
        header = temporary / name
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_text('#include "gameplay_platform.h"\n')
    shutil.copyfile(sdk / "include/zgdk/types.h", temporary / "zgdk/types.h")
    executable = temporary / "test_gameplay"
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=gnu99", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-I", str(temporary),
        "-I", str(root / "tests"), "-I", str(root / "src"),
        *(str(root / "src" / name) for name in ("enemies.c", "bullet.c", "player.c", "objects.c")),
        str(root / "tests/test_gameplay.c"), "-o", str(executable),
    ], check=True)
    subprocess.run([str(executable)], cwd=root, check=True)

"""Run asset-loader regression tests with host sanitizers and mocked hardware."""

from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="enigma-assets-") as directory:
    temporary = Path(directory)
    for name in (
        "zvb_gfx.h", "zar.h", "zos_errors.h", "zgdk/types.h",
        "zgdk/tilemap.h", "zgdk/compress/rle.h", "zgdk/rooms.h",
    ):
        header = temporary / name
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_text('#include "assets_platform.h"\n')
    executable = temporary / "test_assets"
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c99", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-I", str(temporary),
        "-I", str(root / "tests"), "-I", str(root / "src"),
        str(root / "src/assets.c"), str(root / "tests/test_assets.c"),
        "-o", str(executable),
    ], check=True)
    subprocess.run([str(executable)], cwd=root, check=True)

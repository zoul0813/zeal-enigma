#!/usr/bin/env python3
"""Real PS/2 releases during production VRAM uploads, with debugger timing.

The unbatched renderer loses RIGHT's release at the 500-cycle offset.
"""
import argparse
import hashlib
import json
from pathlib import Path
from emulator import Emulator, ROOT, symbol


def advance(emulator, frames):
    for _ in range(frames):
        emulator.frame()


def run(sweep=False, report=None):
    binary_sha256 = hashlib.sha256((ROOT / 'bin/enigma').read_bytes()).hexdigest()
    emulator = Emulator(log_name='verify-keyboard.log')
    cases = [(key, offset) for key in ('RIGHT', 'LEFT', 'UP', 'DOWN')
             for offset in (0, 500, 1500, 3000, 5000)]
    if sweep:
        # Five-microsecond steps can pause inside a bounded DMA bus hold.
        cases += [('RIGHT', offset) for offset in range(0, 16000, 50)
                  if offset not in (0, 500, 1500, 3000, 5000)]
    failures = []
    try:
        emulator.command('run 50000000')
        emulator.type_line('cd H:/')
        emulator.command(f'bp {symbol("draw")}')
        emulator.command(f'bp {symbol("exit")}')
        emulator.type_line('./enigma')
        advance(emulator, 4)
        # Title input uses the same keyboard poll and upload as gameplay,
        # without walls or enemy contact disturbing the experiment.
        for key, offset in cases:
            emulator.command('press ' + key)
            advance(emulator, 4)
            assert emulator.read(symbol('KEYBOARD_keys'), 2) != bytes(2), (key, 'press missing')
            upload = symbol('sprites_render')
            emulator.command(f'bp {upload}')
            emulator.command('s')
            emulator.command('c')
            assert emulator.pc() == upload
            emulator.command(f'bc {upload}')
            if offset:
                emulator.command('run ' + str(offset))
            emulator.command('release ' + key)
            emulator.command('c')
            advance(emulator, 4)
            held = emulator.read(symbol('KEYBOARD_keys'), 2)
            if held != bytes(2):
                failures.append(dict(key=key, offset_tstates=offset, held=list(held)))
                # Repair between independent cases by making/releasing the
                # key while stopped at draw, before the next VRAM upload.
                emulator.command('press ' + key)
                advance(emulator, 4)
                emulator.command('release ' + key)
                advance(emulator, 4)
                assert emulator.read(symbol('KEYBOARD_keys'), 2) == bytes(2), 'Key recovery failed'
        result = dict(binary_sha256=binary_sha256,
                      cases=len(cases), failures=failures)
        if report:
            Path(report).write_text(json.dumps(result, indent=2) + '\n')
        print(f'{"FAIL" if failures else "PASS"}: {len(failures)}/{len(cases)} keyboard releases lost; '
              f'first failures: {failures[:5]}')
        return not failures
    finally:
        emulator.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sweep', action='store_true', help='Sweep RIGHT releases every 50 CPU cycles across the upload')
    parser.add_argument('--report', help='Write per-case JSON evidence')
    args = parser.parse_args()
    raise SystemExit(0 if run(args.sweep, args.report) else 1)

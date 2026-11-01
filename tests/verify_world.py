#!/usr/bin/env python3
"""Headless native-emulator integration checks. Positions are placed near room
edges/objects to exercise transitions and pickups without a full playthrough.
Run from the clone: python3 tests/verify_world.py [--baseline build/baseline].
"""
import argparse
import json
import os
from pathlib import Path
import re
import select
import shutil
import struct
import subprocess
import tempfile
import time
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--baseline', type=Path)
args = parser.parse_args()
image_dir = (ROOT / args.baseline).resolve() if args.baseline else ROOT / 'bin'
map_path = image_dir.parent / 'debug' / 'enigma.map'
if not map_path.exists():
    map_path = image_dir / 'enigma.map'
map_text = map_path.read_text()


def symbol(name):
    match = re.search(r'^\s*([0-9a-fA-F]{8})\s+_' + re.escape(name) + r'\b', map_text, re.M)
    assert match, name
    return int(match[1], 16)


def default_image(env, filename):
    path = Path(os.environ.get(env, str(Path.home() / '.zeal8bit/roms' / filename)))
    assert path.is_file(), f'Missing emulator image: {path}'
    return path


class Emulator:
    def __init__(self):
        self.temp = tempfile.TemporaryDirectory(prefix='enigma-world-')
        work = Path(self.temp.name)
        # The emulator writes these images back; isolate the user's disks.
        for env, filename in [('ZEAL_NATIVE_EEPROM', 'eeprom.img'), ('ZEAL_NATIVE_TF', 'tf.img')]:
            shutil.copyfile(default_image(env, filename), work / filename)
        self.process = subprocess.Popen([
            'zeal-native', '-H', str(image_dir), '--headless', '--console', '--debug',
            '-r', str(default_image('ZEAL_NATIVE_ROM', 'default.img')),
            '-e', str(work / 'eeprom.img'), '-t', str(work / 'tf.img'),
        ], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.held = set()
        self.pending = b''
        self.transcript = ''
        self.receive()

    def receive(self):
        deadline = time.monotonic() + 30
        while b'debug> ' not in self.pending:
            assert time.monotonic() < deadline, 'Emulator timed out'
            readable, _, _ = select.select([self.process.stdout], [], [], 1)
            if not readable:
                continue
            block = os.read(self.process.stdout.fileno(), 65536)
            assert block, 'Emulator closed unexpectedly'
            self.pending += block
        output, self.pending = self.pending.split(b'debug> ', 1)
        text = output.decode(errors='replace')
        self.transcript += text + 'debug> '
        return text

    def command(self, command):
        if command.startswith('press '):
            self.held.add(command.split()[1])
        elif command.startswith('release '):
            self.held.discard(command.split()[1])
        self.process.stdin.write((command + '\n').encode())
        self.process.stdin.flush()
        result = self.receive()
        assert 'Unknown command' not in result, result
        return result

    def type_line(self, text):
        for character in text:
            if character == ':':
                self.command('press SHIFT')
                key = 'SEMICOLON'
            else:
                key = {' ': 'SPACE', '.': 'PERIOD', '/': 'SLASH'}.get(character, character.upper())
            self.command('press ' + key)
            self.command('run 50000')
            self.command('release ' + key)
            self.command('run 50000')
            if character == ':':
                self.command('release SHIFT')
        self.command('press ENTER')
        self.command('run 50000')
        self.command('release ENTER')
        self.command('run 40000000')

    def read(self, address, size):
        data = bytearray()
        for offset in range(0, size, 240):
            text = self.command(f'rb {address + offset} {min(240, size - offset)}')
            for match in re.finditer(r'\[DBG\] [0-9a-fA-F]{4}: ((?:[0-9a-fA-F]{2} ?)+)', text):
                data.extend(bytes.fromhex(match[1]))
        assert len(data) == size, (address, size, len(data))
        return bytes(data)

    def write(self, address, data):
        for offset in range(0, len(data), 6):
            chunk = data[offset:offset + 6]
            self.command(f'wb {address + offset} ' + ' '.join(str(x) for x in chunk))

    def pc(self):
        text = self.command('regs')
        return int(re.search(r'PC=([0-9a-fA-F]+)', text)[1], 16)

    def frame(self):
        self.command('s')  # leave the current draw breakpoint
        self.command('run 30000000')
        assert self.pc() == symbol('draw'), 'Game exited or failed to return to draw()'

    def release(self):
        for key in sorted(self.held):
            self.command('release ' + key)
        self.frame()

    def close(self):
        self.process.stdin.write(b'quit\n')
        self.process.stdin.flush()
        self.process.communicate(timeout=10)
        (ROOT / 'build').mkdir(exist_ok=True)
        suffix = 'baseline' if args.baseline else 'world-lib'
        (ROOT / 'build' / f'verify-{suffix}.log').write_text(self.transcript)
        self.temp.cleanup()


def objects(emulator, name, count):
    data = emulator.read(symbol(name), count * 5)
    return [dict(zip(['room', 'id', 'x', 'y', 'flags'], data[i:i + 5])) for i in range(0, len(data), 5)]


xml = ET.parse(ROOT / 'assets/enigma.tmx').getroot()
terrain = [int(x) for x in xml.find("layer[@id='3']/data").text.replace('\n', '').split(',') if x.strip()]


def position(emulator):
    return tuple(emulator.read(symbol('world_position'), 2))


def player_position(emulator):
    return struct.unpack('<HH', emulator.read(symbol('player'), 4))


def set_player(emulator, x, y):
    emulator.write(symbol('player'), struct.pack('<HH', x, y))


def scene(emulator):
    x, y = position(emulator)
    room_id = y * 8 + x
    tilemap = int.from_bytes(emulator.read(symbol('_TILEMAP'), 2), 'little')
    tiles, ox, oy, width, height = struct.unpack('<HHHHH', emulator.read(tilemap, 10))
    assert (ox, oy, width, height) == (0, 4, 40, 21)
    actual = emulator.read(tiles, 840)
    expected = bytearray(0 if terrain[(y * 21 + row) * 320 + x * 40 + col] == 1 else 1
                         for row in range(21) for col in range(40))
    keys, fuel, gates = objects(emulator, 'keys', 9), objects(emulator, 'fuel', 38), objects(emulator, 'gates', 32)
    for index, key in enumerate(keys):
        if key['room'] == room_id and key['flags'] == 1:
            expected[key['y'] * 40 + key['x']] = 96 + index
    for can in fuel:
        if can['room'] == room_id and can['flags'] == 1:
            expected[can['y'] * 40 + can['x']] = 5
    for gate in gates:
        if gate['room'] == room_id and gate['flags'] & 1:
            for i in range(8):
                vertical = bool(gate['flags'] & 128)
                tile_x = gate['x'] + (0 if vertical else i)
                tile_y = gate['y'] + (i if vertical else 0)
                expected[tile_y * 40 + tile_x] = 80 + gate['id']
    assert actual == expected, f'Logical terrain/object overlay mismatch in room {(x, y)}'
    enemy_count = emulator.read(symbol('ENEMY_ROOMS') + room_id, 1)[0]
    enemies = emulator.read(symbol('ENEMIES'), 8 * 19)
    active = 0
    for i in range(8):
        sprite = int.from_bytes(enemies[i * 19 + 15:i * 19 + 17], 'little')
        active += emulator.read(sprite + 4, 1)[0] != 0
    assert active == enemy_count, (room_id, active, enemy_count)
    return actual


def transition(emulator, direction):
    emulator.release()
    previous = position(emulator)
    if direction == 'RIGHT':
        set_player(emulator, 637, 160); expected = (previous[0] + 1, previous[1]); arrival = (16, 160)
    elif direction == 'LEFT':
        set_player(emulator, 17, 160); expected = (previous[0] - 1, previous[1]); arrival = (640, 160)
    elif direction == 'UP':
        set_player(emulator, 160, 81); expected = (previous[0], previous[1] - 1); arrival = (160, 384)
    else:
        set_player(emulator, 160, 383); expected = (previous[0], previous[1] + 1); arrival = (160, 80)
    emulator.command('press ' + direction)
    for _ in range(12):
        emulator.frame()
        if position(emulator) == expected:
            break
    assert position(emulator) == expected, (direction, previous, position(emulator), player_position(emulator), list(emulator.read(symbol('player')+12,2)))
    assert player_position(emulator) == arrival, (direction, player_position(emulator), arrival)
    scene(emulator)


def run():
    emulator = Emulator()
    try:
        emulator.command('run 50000000')
        emulator.type_line('cd H:/')
        emulator.command(f'bp {symbol("draw")}')
        emulator.command(f'bp {symbol("exit")}')
        executable = 'enigma.bin' if (image_dir / 'enigma.bin').exists() else 'enigma'
        emulator.type_line('./' + executable)
        assert emulator.pc() == symbol('draw'), 'Initialization failed'
        assert position(emulator) == (2, 3)
        initial_scene = scene(emulator)
        initial_player = emulator.read(symbol('player'), 22)
        assert player_position(emulator) == (160, 336)
        baseline_json = ROOT / 'build' / 'baseline-snapshot.json'
        snapshot = {'tiles': list(initial_scene), 'player': list(initial_player)}
        if args.baseline:
            baseline_json.write_text(json.dumps(snapshot))
        else:
            assert json.loads(baseline_json.read_text()) == snapshot, 'Startup differs from baseline'
            assert '_WORLD_TRANSITION_SLIDE' not in map_text and '_minimap_render' not in map_text
            assert '_world_runtime' in map_text, 'world.lib core missing'

        emulator.command('press RIGHT'); emulator.command('press SPACE'); emulator.frame()
        assert emulator.read(symbol('player') + 15, 1)[0] == 107, 'Shooting did not consume energy'
        bullets = emulator.read(symbol('BULLETS'), 16)
        assert any(emulator.read(int.from_bytes(bullets[i + 2:i + 4], 'little') + 4, 1)[0] != 0 for i in range(0, 16, 4))
        emulator.release()

        set_player(emulator, 185, 248)
        emulator.command('press RIGHT'); emulator.frame(); emulator.release()
        assert emulator.read(symbol('player') + 16, 2) == b'\x04\x00', 'Key not collected'
        assert int.from_bytes(emulator.read(symbol('player') + 18, 4), 'little') == 5000
        assert objects(emulator, 'keys', 9)[2]['flags'] == 0
        assert all(g['flags'] == 0 for g in objects(emulator, 'gates', 32) if g['id'] == 2)
        scene(emulator)

        emulator.write(symbol('player') + 15, bytes([50]))
        set_player(emulator, 553, 128)
        emulator.command('press RIGHT'); emulator.frame(); emulator.release()
        assert emulator.read(symbol('player') + 15, 1)[0] == 108, 'Fuel not collected'
        assert all(c['flags'] == 0 for c in objects(emulator, 'fuel', 38) if c['room'] == 26)
        scene(emulator)

        for direction in ['DOWN', 'UP', 'RIGHT', 'LEFT', 'UP', 'DOWN']:
            transition(emulator, direction)
        assert position(emulator) == (2, 3)
        scene(emulator)  # pickups stay removed after leaving/re-entering
        emulator.release()
        bullets = emulator.read(symbol('BULLETS'), 16)
        assert all(emulator.read(int.from_bytes(bullets[i + 2:i + 4], 'little') + 4, 1)[0] == 0 for i in range(0, 16, 4))

        if not args.baseline:
            # Isolate world-boundary clamping from terrain and navigation by
            # putting the game at each grid edge while preserving the viewport.
            for direction, grid, xy, expected in [
                ('LEFT', (0, 3), (17, 160), (16, 160)),
                ('RIGHT', (7, 3), (637, 160), (640, 160)),
                ('UP', (2, 0), (160, 81), (160, 80)),
                ('DOWN', (2, 7), (160, 383), (160, 384)),
            ]:
                emulator.release()
                emulator.write(symbol('world_position'), bytes(grid)); set_player(emulator, *xy)
                emulator.command('press ' + direction)
                emulator.frame(); emulator.frame()
                assert position(emulator) == grid and player_position(emulator) == expected
        print('PASS:', 'baseline' if args.baseline else 'world.lib',
              'startup/map parity, keys/gates, fuel, shooting, enemies, four-way transitions, arrival positions, and persistent pickups')
    finally:
        emulator.close()


if __name__ == '__main__':
    run()

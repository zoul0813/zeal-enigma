"""Shared headless Zeal debugger harness for the current Enigma build.

Writable emulator disks are copied into a temporary directory. Tests read the
matching target link map, and keep a transcript under build/ for diagnosis.
"""
import os
from pathlib import Path
import re
import select
import shutil
import struct
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
image_dir = ROOT / 'bin'
map_path = ROOT / 'debug/enigma.map'
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
    def __init__(self, log_name="verify-world-lib.log"):
        self.log_name = log_name
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
        try:
            if self.process.poll() is None:
                self.process.stdin.write(b'quit\n')
                self.process.stdin.flush()
                try:
                    self.process.communicate(timeout=10)
                except subprocess.TimeoutExpired:
                    self.process.kill()
                    self.process.communicate()
        finally:
            (ROOT / 'build').mkdir(exist_ok=True)
            (ROOT / 'build' / self.log_name).write_text(self.transcript)
            self.temp.cleanup()


def position(emulator):
    return tuple(emulator.read(symbol('world_position'), 2))


def player_position(emulator):
    return struct.unpack('<HH', emulator.read(symbol('player'), 4))


def set_player(emulator, x, y):
    emulator.write(symbol('player'), struct.pack('<HH', x, y))

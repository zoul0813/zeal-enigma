#!/usr/bin/env python3
"""Target emulator checks for the original-game mechanics, using the existing
safe disk-copy/debugger harness. No C64/C16 or old Zeal baseline is asserted.
"""
import struct
from emulator import Emulator, symbol, position, player_position, set_player


def byte(emulator, name, offset=0):
    return emulator.read(symbol(name) + offset, 1)[0]


def enemy_sprite(emulator, index, right=False):
    offset = 17 if right else 15
    return int.from_bytes(emulator.read(symbol('ENEMIES') + index * 19 + offset, 2), 'little')


def hide_enemies(emulator):
    for i in range(16):
        for right in (False, True):
            emulator.write(enemy_sprite(emulator, i, right) + 4, b'\0')


def active_enemies(emulator):
    return sum(emulator.read(enemy_sprite(emulator, i) + 4, 1)[0] != 0 for i in range(16))


def advance(emulator, frames=1):
    for _ in range(frames):
        emulator.frame()


def until_state(emulator, state, maximum=90):
    for _ in range(maximum):
        if byte(emulator, 'game_state') == state:
            return
        emulator.frame()
    raise AssertionError(('state timeout', state, byte(emulator, 'game_state')))


def press(emulator, key):
    emulator.command('press ' + key)
    advance(emulator, 2)
    emulator.command('release ' + key)
    advance(emulator, 2)


def force_contact(emulator):
    hide_enemies(emulator)
    player = player_position(emulator)
    emulator.write(symbol('ENEMIES'), struct.pack('<HH', *player))
    emulator.write(enemy_sprite(emulator, 0), struct.pack('<HHB', *player, 48))
    emulator.write(enemy_sprite(emulator, 0, True), struct.pack('<HHB', player[0] + 16, player[1], 49))
    until_state(emulator, 2, 4)


def run():
    emulator = Emulator(log_name="verify-gameplay.log")
    try:
        emulator.command('run 50000000')
        emulator.type_line('cd H:/')
        emulator.command(f'bp {symbol("draw")}')
        emulator.command(f'bp {symbol("exit")}')
        emulator.type_line('./enigma')
        assert emulator.pc() == symbol('draw'), 'Initialization failed'
        assert byte(emulator, 'game_state') == 0
        assert byte(emulator, 'game_difficulty') == 1
        press(emulator, 'RIGHT')
        assert byte(emulator, 'game_difficulty') == 2
        press(emulator, 'LEFT')
        assert byte(emulator, 'game_difficulty') == 1
        press(emulator, 'SPACE')
        assert byte(emulator, 'game_state') == 1
        assert position(emulator) == (3, 3)
        assert player_position(emulator) == (112, 320)
        assert byte(emulator, 'player', 14) == 3
        assert byte(emulator, 'player', 15) == 215
        assert active_enemies(emulator) == 2
        hide_enemies(emulator)

        # Fire repeats while held, and does not consume energy while stationary.
        emulator.command('press SPACE')
        advance(emulator, 5)
        assert byte(emulator, 'player', 15) == 215
        emulator.command('press RIGHT')
        advance(emulator, 9)
        energy = byte(emulator, 'player', 15)
        assert 210 <= energy <= 212, energy
        emulator.release()

        # Enter original key room via its right-hand entrance.
        set_player(emulator, 17, 160)
        emulator.command('press LEFT')
        for _ in range(4):
            emulator.frame()
            if position(emulator) == (2, 3):
                break
        assert position(emulator) == (2, 3)
        assert player_position(emulator) == (640, 160)
        emulator.release()
        hide_enemies(emulator)
        set_player(emulator, 185, 248)
        press(emulator, 'RIGHT')
        assert emulator.read(symbol('player') + 16, 2) == b'\x04\0'
        assert int.from_bytes(emulator.read(symbol('player') + 18, 4), 'little') == 5000
        assert emulator.read(symbol('keys') + 2 * 5 + 4, 1) == b'\0'

        # Death costs one life, preserves key progress, and restores entry/ammo.
        emulator.write(symbol('player') + 15, b'\x32')
        force_contact(emulator)
        assert byte(emulator, 'player', 14) == 2
        advance(emulator, 10)
        assert byte(emulator, 'player', 14) == 2, 'Repeated life loss during death pause'
        until_state(emulator, 1)
        assert position(emulator) == (2, 3)
        assert player_position(emulator) == (640, 160)
        assert byte(emulator, 'player', 15) == 215
        assert emulator.read(symbol('player') + 16, 2) == b'\x04\0'
        assert emulator.read(symbol('keys') + 2 * 5 + 4, 1) == b'\0'
        assert active_enemies(emulator) == 12

        for expected in (1, 0):
            force_contact(emulator)
            assert byte(emulator, 'player', 14) == expected
            until_state(emulator, 1 if expected else 0)
        assert byte(emulator, 'game_state') == 0
        assert int.from_bytes(emulator.read(symbol('high_score'), 4), 'little') == 5000
        press(emulator, 'SPACE')
        assert position(emulator) == (3, 3)
        assert byte(emulator, 'player', 14) == 3
        assert byte(emulator, 'player', 15) == 215
        assert emulator.read(symbol('player') + 16, 6) == bytes(6)
        assert emulator.read(symbol('keys') + 2 * 5 + 4, 1) == b'\x01'
        assert int.from_bytes(emulator.read(symbol('high_score'), 4), 'little') == 5000
        print('PASS: target title/difficulty, original start/counts, held/stationary fire, room entry, key persistence, three contact deaths, entry respawn/ammo refill, game over/restart, retained high score')
    finally:
        emulator.close()


if __name__ == '__main__':
    run()

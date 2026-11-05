#!/usr/bin/env python3
"""Target emulator checks for the original-game mechanics, using the existing
safe disk-copy/debugger harness. No C64/C16 or old Zeal baseline is asserted.
"""
import struct
from emulator import ENEMY_STRIDE, ENEMY_TAG, ENEMY_LEFT, ENEMY_RIGHT
from emulator import BULLET_STRIDE, BULLET_TAG, BULLET_SPRITE
from emulator import Emulator, ROOT, symbol, position, player_position, set_player


def byte(emulator, name, offset=0):
    return emulator.read(symbol(name) + offset, 1)[0]


def enemy_sprite(emulator, index, right=False):
    offset = ENEMY_RIGHT if right else ENEMY_LEFT
    return int.from_bytes(emulator.read(symbol('ENEMIES') + index * ENEMY_STRIDE + offset, 2), 'little')


def hide_enemies(emulator):
    for i in range(16):
        emulator.write(symbol('ENEMIES') + i*ENEMY_STRIDE + ENEMY_TAG, b'\xff')
        for right in (False, True):
            emulator.write(enemy_sprite(emulator, i, right) + 4, b'\0')


def active_enemies(emulator):
    return sum(emulator.read(enemy_sprite(emulator, i) + 4, 1)[0] != 0 for i in range(16))


def verify_enemy_artwork(emulator):
    tiles = (ROOT / 'build/assets/enemies.zts').read_bytes()
    def visible(index):
        tile = byte(emulator, 'ENEMIES', index * ENEMY_STRIDE + ENEMY_TAG)
        if tile == 255:
            return
        for right in (False, True):
            graphic = emulator.read(enemy_sprite(emulator, index, right) + 4, 1)[0] - 48
            assert 0 <= graphic < len(tiles) // 128, ('enemy artwork out of range', tile, graphic)
            assert any(tiles[graphic * 128:(graphic + 1) * 128]), ('invisible enemy', tile, graphic)
    # Check actual room spawns before replacing them with controlled types.
    for index in range(16):
        visible(index)
    hide_enemies(emulator)
    emulator.write(symbol('ENEMIES'), struct.pack('<HH', 600, 100))
    for kind in range(8):
        emulator.write(symbol('ENEMIES') + ENEMY_TAG, bytes((kind,)))
        graphics = set()
        for frame in range(23):
            emulator.frame()
            if frame < 3:
                continue  # Let the alternating enemy pass select this type.
            visible(0)
            graphics.add(emulator.read(enemy_sprite(emulator, 0) + 4, 1)[0])
        assert len(graphics) == 2, ('enemy animation missing', kind, graphics)


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
    # Allow the emulated keyboard to deliver its scan code before release.
    advance(emulator, 3)
    emulator.command('release ' + key)
    advance(emulator, 2)


def force_contact(emulator):
    hide_enemies(emulator)
    # Isolate contact from leftover shots, regardless of the actor phase.
    for i in range(8):
        bullet = symbol('BULLETS') + i * BULLET_STRIDE
        emulator.write(bullet + BULLET_TAG, b'\xff')
        sprite = int.from_bytes(emulator.read(bullet + BULLET_SPRITE, 2), 'little')
        emulator.write(sprite + 4, b'\0')
    player = player_position(emulator)
    emulator.write(symbol('ENEMIES'), struct.pack('<HH', *player))
    emulator.write(symbol('ENEMIES') + ENEMY_TAG, b'\0')
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
        advance(emulator, 4)  # Let the SDK observe neutral input after startup.
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
        verify_enemy_artwork(emulator)
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

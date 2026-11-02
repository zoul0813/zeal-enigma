#!/usr/bin/env python3
"""Current-port world.lib integration checks using the headless Zeal emulator.

These compare logical rooms with TMX terrain and live object state, not a
historical executable snapshot. Player/enemy positions are controlled to test
world ownership and persistent pickups independently of combat.
"""
import struct
import xml.etree.ElementTree as ET
from emulator import Emulator, ROOT, image_dir, map_text, symbol, position, player_position, set_player

xml = ET.parse(ROOT / 'assets/enigma.tmx').getroot()
terrain = [int(x) for x in xml.find("layer[@id='3']/data").text.replace('\n', '').split(',') if x.strip()]
# Level1 population checks from original $2380 (TMX reverses world Y).
expected_populations = {(3,3):2, (2,3):12, (2,4):10, (2,2):8}


def objects(emulator, name, count):
    data = emulator.read(symbol(name), count * 5)
    return [dict(zip(['room', 'id', 'x', 'y', 'flags'], data[i:i + 5])) for i in range(0, len(data), 5)]


def enemy_sprites(emulator):
    enemies = emulator.read(symbol('ENEMIES'), 16 * 19)
    return [(int.from_bytes(enemies[i + 15:i + 17], 'little'),
             int.from_bytes(enemies[i + 17:i + 19], 'little')) for i in range(0, len(enemies), 19)]


def hide_enemies(emulator):
    for left, right in enemy_sprites(emulator):
        emulator.write(left + 4, b'\0')
        emulator.write(right + 4, b'\0')


def scene(emulator, check_population=False):
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
    if check_population:
        active = sum(emulator.read(left + 4, 1)[0] != 0 for left, _ in enemy_sprites(emulator))
        assert active == expected_populations[(x,y)], ((x,y), active, expected_populations[(x,y)])
    return actual


def release_controls(emulator):
    emulator.release()
    # Debugger key events traverse the emulated keyboard interrupt before the
    # input poll sees them. Settle release before teleporting a test position.
    for _ in range(5):
        if emulator.read(symbol('player') + 12, 2) == b'\0\0':
            return
        emulator.frame()
    raise AssertionError('Released controls did not settle')


def transition(emulator, direction):
    hide_enemies(emulator)
    release_controls(emulator)
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
    assert position(emulator) == expected, (direction, previous, position(emulator), player_position(emulator))
    assert player_position(emulator) == arrival, (direction, player_position(emulator), arrival)
    scene(emulator, check_population=True)
    hide_enemies(emulator)
    release_controls(emulator)


def move_to_pickup(emulator, x, y, name, offset):
    hide_enemies(emulator)
    release_controls(emulator)
    set_player(emulator, x, y)
    emulator.command('press RIGHT')
    for _ in range(4):
        emulator.frame()
        if emulator.read(symbol(name) + offset, 1) == b'\0':
            break
    release_controls(emulator)
    assert emulator.read(symbol(name) + offset, 1) == b'\0', ('pickup', name, offset)


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
        assert emulator.read(symbol('game_state'), 1) == b'\0', 'Title screen missing'
        emulator.command('press SPACE')
        for _ in range(3):
            emulator.frame()
            if emulator.read(symbol('game_state'), 1) == b'\x01':
                break
        release_controls(emulator)
        assert position(emulator) == (3,3)
        assert player_position(emulator) == (112,320)
        scene(emulator, check_population=True)
        assert '_WORLD_TRANSITION_SLIDE' not in map_text and '_minimap_render' not in map_text
        assert '_world_runtime' in map_text, 'world.lib core missing'

        transition(emulator, 'LEFT')
        move_to_pickup(emulator, 185, 248, 'keys', 2 * 5 + 4)
        assert emulator.read(symbol('player') + 16, 2) == b'\x04\x00', 'Key not recorded'
        assert int.from_bytes(emulator.read(symbol('player') + 18, 4), 'little') == 5000
        assert all(g['flags'] == 0 for g in objects(emulator, 'gates', 32) if g['id'] == 2)
        scene(emulator)

        fuel = objects(emulator, 'fuel', 38)
        fuel_index = next(i for i, can in enumerate(fuel) if can['room'] == 26)
        emulator.write(symbol('player') + 15, bytes([50]))
        move_to_pickup(emulator, 553, 128, 'fuel', fuel_index * 5 + 4)
        assert emulator.read(symbol('player') + 15, 1)[0] == 215, 'Fuel did not refill laser'
        scene(emulator)

        for direction in ['DOWN', 'UP', 'RIGHT', 'LEFT', 'UP', 'DOWN']:
            transition(emulator, direction)
        assert position(emulator) == (2,3)
        assert objects(emulator, 'keys', 9)[2]['flags'] == 0
        assert objects(emulator, 'fuel', 38)[fuel_index]['flags'] == 0
        scene(emulator)  # Room reload preserves removed pickups and unlocked gates.
        bullets = emulator.read(symbol('BULLETS'), 8 * 4)
        assert all(emulator.read(int.from_bytes(bullets[i + 2:i + 4], 'little') + 4, 1)[0] == 0 for i in range(0, len(bullets), 4))

        # Isolate world-boundary clamping from terrain and navigation by placing
        # the world identity at each grid edge while retaining the room viewport.
        for direction, grid, xy, expected in [
            ('LEFT', (0,3), (17,160), (16,160)),
            ('RIGHT', (7,3), (637,160), (640,160)),
            ('UP', (2,0), (160,81), (160,80)),
            ('DOWN', (2,7), (160,383), (160,384)),
        ]:
            hide_enemies(emulator)
            release_controls(emulator)
            emulator.write(symbol('world_position'), bytes(grid))
            set_player(emulator, *xy)
            emulator.command('press ' + direction)
            emulator.frame(); emulator.frame()
            assert position(emulator) == grid and player_position(emulator) == expected
        print('PASS: target world.lib startup, 840-tile rooms/overlays, original enemy populations, keys/gates/fuel persistence, four-way transitions/arrivals, bullet clearing and world boundary clamping')
    finally:
        emulator.close()


if __name__ == '__main__':
    run()

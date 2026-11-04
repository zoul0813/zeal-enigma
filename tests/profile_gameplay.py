#!/usr/bin/env python3
"""Profile the uninstrumented Z80 binary using debugger cycle timestamps.

Emulated 10 MHz CPU time, not host wall time. The synthetic stress cases reset
actor positions/projectiles between frames; they do not alter executable code.
Requires the matching SDCC build listings to locate private state variables.
"""
import hashlib
import json
from pathlib import Path
import re
import shutil
import statistics
import struct
from emulator import ENEMY_STRIDE, ENEMY_LEFT, ENEMY_RIGHT, BULLET_STRIDE, BULLET_SPRITE, BULLET_TAG
from emulator import Emulator, ROOT, symbol, position, set_player

CPU_HZ = 10_000_000
VIDEO_TSTATES = 167_680  # 800 pixels * 524 lines * 40 ns at 10 MHz.
SAMPLES = 120


def private(module, name, anchor):
    text = (ROOT / f'build/CMakeFiles/enigma.dir/src/{module}.c.sym').read_text()
    def offset(label):
        match = re.search(r'^\s*\d+\s+_' + label + r'\s+([0-9A-Fa-f]+)\s+G?R', text, re.M)
        assert match, label
        return int(match[1], 16)
    return symbol(anchor) + offset(name) - offset(anchor)


POINTS = ['input', 'update', 'draw', 'bullet_move', 'enemies_move',
          'player_move', 'sprites_render', 'gfx_wait_end_vblank']
ADDRESSES = {symbol(name): name for name in POINTS + ['load_tilemap', 'enemies_spawn']}


def continued(emulator):
    output = emulator.command('c')
    match = re.search(r'Paused @ 0x([0-9a-f]+) \(cyc=(\d+)\)', output)
    assert match, output
    address, cycles = int(match[1], 16), int(match[2])
    assert address in ADDRESSES, ('Unexpected stop', address, output)
    return ADDRESSES[address], cycles


def summarize(values):
    ordered = sorted(values)
    return dict(mean_ms=round(statistics.mean(values) / 10000, 4),
                p95_ms=round(ordered[min(len(ordered)-1, int(len(ordered)*.95))] / 10000, 4),
                max_ms=round(max(values) / 10000, 4))


def byte(emulator, name, offset=0):
    return emulator.read(symbol(name) + offset, 1)[0]


def actors(emulator):
    raw = emulator.read(symbol('ENEMIES'), 16 * ENEMY_STRIDE)
    return [(symbol('ENEMIES') + i * ENEMY_STRIDE,
             int.from_bytes(raw[i*ENEMY_STRIDE+ENEMY_LEFT:i*ENEMY_STRIDE+ENEMY_LEFT+2], 'little'),
             int.from_bytes(raw[i*ENEMY_STRIDE+ENEMY_RIGHT:i*ENEMY_STRIDE+ENEMY_RIGHT+2], 'little')) for i in range(16)]


def active_count(emulator, enemies):
    return sum(emulator.read(left + 4, 1)[0] != 0 for _, left, _ in enemies)


def run_frames(emulator, count):
    for _ in range(count):
        name, cycles = continued(emulator)
        assert name == 'draw'
    return cycles


def release(emulator):
    for key in sorted(emulator.held):
        emulator.command('release ' + key)
    run_frames(emulator, 4)


def enter(emulator, x, y):
    # Controlled entry through a real world_move/load/on_enter, using the same
    # technique as the navigation regression checks. No loader is bypassed.
    release(emulator)
    if x:
        emulator.write(symbol('world_position'), bytes((x-1, y)))
        set_player(emulator, 637, 160)
        key = 'RIGHT'
    else:
        emulator.write(symbol('world_position'), bytes((1, y)))
        set_player(emulator, 17, 160)
        key = 'LEFT'
    previous = run_frames(emulator, 1)
    for function in ('load_tilemap', 'enemies_spawn'):
        emulator.command(f'bp {symbol(function)}')
    emulator.command('press ' + key)
    intervals, spans, pending = [], {}, {}
    for _ in range(30):
        name, cycles = continued(emulator)
        if name in ('load_tilemap', 'enemies_spawn'):
            sp = int(re.search(r'SP=([0-9a-fA-F]+)', emulator.command('regs'))[1], 16)
            address = int.from_bytes(emulator.read(sp, 2), 'little')
            pending[name] = (cycles, address)
            ADDRESSES[address] = name + '_return'
            emulator.command(f'bp {address}')
        elif name.endswith('_return'):
            function = name[:-7]
            started, address = pending.pop(function)
            spans[function + '_ms'] = round((cycles-started)/10000, 4)
            emulator.command(f'bc {address}')
            del ADDRESSES[address]
        else:
            assert name == 'draw'
            intervals.append((cycles-previous)/10000)
            previous = cycles
            if position(emulator) == (x, y):
                break
    assert position(emulator) == (x, y)
    for function in ('load_tilemap', 'enemies_spawn'):
        emulator.command(f'bc {symbol(function)}')
    release(emulator)
    result = dict(room=[x,y], longest_frame_ms=round(max(intervals),4), **spans)
    print(json.dumps({'room_entry': result}), flush=True)
    return result


def fixture(emulator, enemies, bullets=False):
    # Use valid locations in the loaded room, spread across the half opposite
    # the player. Reset before every frame to keep a stable, active workload.
    tilemap = int.from_bytes(emulator.read(symbol('_TILEMAP'), 2), 'little')
    tiles = int.from_bytes(emulator.read(tilemap, 2), 'little')
    terrain = emulator.read(tiles, 840)
    free = [(x, y) for y in range(1,20) for x in range(1,38)
            if terrain[y*40+x] == terrain[y*40+x+1] == 0]
    assert len(free) > 32
    player_tile = min(free, key=lambda p: p[0]+p[1])
    target = ((player_tile[0]+1)*16, (player_tile[1]+5)*16)
    choices = sorted(free, key=lambda p: abs(p[0]-player_tile[0])+abs(p[1]-player_tile[1]), reverse=True)
    chosen = choices[:16]
    bullet_raw = emulator.read(symbol('BULLETS'), 8 * BULLET_STRIDE)
    bullet_sprites = [int.from_bytes(bullet_raw[i*BULLET_STRIDE+BULLET_SPRITE:i*BULLET_STRIDE+BULLET_SPRITE+2], 'little') for i in range(8)]
    count = active_count(emulator, enemies)
    delay = private('enemies', 'enemy_delay', 'ENEMIES')
    def reset():
        set_player(emulator, *target)
        emulator.write(delay, b'\0')
        for index, (base, left, right) in enumerate(enemies[:count]):
            x, y = chosen[index]
            x, y = (x+1)*16, (y+5)*16
            emulator.write(base, struct.pack('<HH', x, y))
            emulator.write(left, struct.pack('<HHB', y, x, 48+(index&7)*4))
            emulator.write(right, struct.pack('<HHB', y, x+16, 49+(index&7)*4))
        if bullets:
            # Set a valid pool high-water mark and live tags for the synthetic
            # eight-projectile workload, retaining actual sprite reservations.
            emulator.write(private('bullet', 'bullet_pool', 'BULLETS') + 2,
                           struct.pack('<H', symbol('BULLETS') + 8 * BULLET_STRIDE))
            # Eight non-hitting in-flight bullets exercise all enemy hit tests.
            for i, sprite in enumerate(bullet_sprites):
                emulator.write(symbol('BULLETS') + i*BULLET_STRIDE, bytes((1, 0)))
                emulator.write(symbol('BULLETS') + i*BULLET_STRIDE + BULLET_TAG, b'\0')
                emulator.write(sprite, struct.pack('<HHB', target[1], target[0], 13))
    reset.bullet_sprites = bullet_sprites if bullets else []
    return reset, count


def profile(emulator, label, fixture_fn=None, limit=SAMPLES, stop_on_death=False):
    for name in POINTS:
        if name != 'gfx_wait_end_vblank':
            emulator.command(f'bp {symbol(name)}')
    while True:
        name, start = continued(emulator)
        if name == 'input':
            break
    frames = []
    for _ in range(limit):
        emulator.command(f'bp {symbol("gfx_wait_end_vblank")}')
        if fixture_fn:
            fixture_fn()
        waiting = emulator.read(private('enemies', 'enemy_delay', 'ENEMIES'), 1)[0] != 0
        phases = {'input': start}
        while True:
            name, cycles = continued(emulator)
            if name == 'input':
                break
            assert name not in phases, ('Nested timing phase', label, name)
            phases[name] = cycles
            if name == 'gfx_wait_end_vblank':
                emulator.command(f'bc {symbol(name)}')
        if fixture_fn:
            assert 'player_move' in phases, (label, 'Missing per-frame player update')
            assert ('enemies_move' in phases) != ('bullet_move' in phases), (label, 'Actor phases must alternate')
        died = byte(emulator, 'game_state') == 2
        assert stop_on_death or not died, (label, 'Contact/death changed stress case')
        bullet_count = sum(emulator.read(sprite+4,1)[0] != 0 for sprite in getattr(fixture_fn, 'bullet_sprites', []))
        frames.append({'frame': cycles-start, 'waiting_for_enemies': waiting, 'stress_bullets_after_frame': bullet_count,
                       'logic': phases['draw']-start,
                       'upload': phases['gfx_wait_end_vblank']-phases['sprites_render'],
                       'update': phases['draw']-phases['update'] if 'update' in phases else None,
                       'bullets': phases.get('enemies_move', phases.get('player_move'))-phases['bullet_move'] if 'bullet_move' in phases else None,
                       'enemies': phases['player_move']-phases['enemies_move'] if 'enemies_move' in phases else None})
        start = cycles
        if died:
            break
    # Restore the navigation harness to its usual single draw breakpoint.
    for name in POINTS:
        if name != 'draw':
            emulator.command(f'bc {symbol(name)}')
    continued(emulator)
    result = {'name': label, 'frames': len(frames), 'ended_on_contact': died,
              'render_fps': round(CPU_HZ*len(frames)/sum(f['frame'] for f in frames), 3),
              'logic_hz': round(CPU_HZ*sum(f['update'] is not None for f in frames)/sum(f['frame'] for f in frames), 3),
              'enemy_phase_hz': round(CPU_HZ*sum(f['enemies'] is not None for f in frames)/sum(f['frame'] for f in frames), 3),
              'bullet_phase_hz': round(CPU_HZ*sum(f['bullets'] is not None for f in frames)/sum(f['frame'] for f in frames), 3),
              'over_1_5_video_frames': sum(f['frame'] > VIDEO_TSTATES*1.5 for f in frames)}
    for field in ('frame','logic','upload','update','bullets','enemies'):
        values = [f[field] for f in frames if f[field] is not None]
        if values:
            result[field] = summarize(values)
    if fixture_fn:
        enemy_frames = [f['enemies'] is not None for f in frames]
        assert all(a != b for a,b in zip(enemy_frames,enemy_frames[1:])), 'Actor phase did not alternate'
    active = [f for f in frames if not f['waiting_for_enemies']]
    if active:
        result['after_entry_delay'] = dict(frames=len(active), render_fps=round(CPU_HZ*len(active)/sum(f['frame'] for f in active),3), worst_frame_ms=round(max(f['frame'] for f in active)/10000,4))
    if getattr(fixture_fn, 'bullet_sprites', []):
        result['stress_bullets_after_frame'] = dict(minimum=min(f['stress_bullets_after_frame'] for f in frames), maximum=max(f['stress_bullets_after_frame'] for f in frames))
        assert result['stress_bullets_after_frame']['minimum'] == 8, 'Stress bullets did not survive the frame'
    result['raw_frames_tstates'] = frames
    print(json.dumps({k:v for k,v in result.items() if k != 'raw_frames_tstates'}), flush=True)
    return result


def run():
    emulator = Emulator(log_name='profile-gameplay.log')
    results = []
    try:
        emulator.command('run 50000000'); emulator.type_line('cd H:/')
        emulator.command(f'bp {symbol("draw")}'); emulator.command(f'bp {symbol("exit")}')
        emulator.type_line('./enigma')
        assert byte(emulator, 'game_state') == 0
        results.append(profile(emulator, 'title'))
        emulator.command('press SPACE'); run_frames(emulator, 2); release(emulator)
        enemies = actors(emulator)
        assert active_count(emulator, enemies) == 2
        results.append(profile(emulator, 'starting room: natural idle play', limit=120, stop_on_death=True))
        while byte(emulator, 'game_state') == 2:
            run_frames(emulator, 1)
        reset, _ = fixture(emulator, enemies)
        results.append(profile(emulator, 'starting room: 2 active enemies', reset))
        emulator.command('press RIGHT'); emulator.command('press SPACE')
        results.append(profile(emulator, 'starting room: 2 enemies, sustained fire', reset))
        release(emulator)
        # Level1 also uses the original maximum of16 enemies in this room.
        entry = enter(emulator, 6, 0)
        assert active_count(emulator, enemies) == 16
        results.append(profile(emulator, 'crowded room: natural idle play', limit=240, stop_on_death=True))
        while byte(emulator, 'game_state') == 2:
            run_frames(emulator, 1)
        reset, _ = fixture(emulator, enemies)
        results.append(profile(emulator, 'crowded room: 16 active enemies', reset))
        reset, _ = fixture(emulator, enemies, bullets=True)
        results.append(profile(emulator, 'crowded room: 16 enemies, 8 in-flight bullets', reset))
        entries = [entry, enter(emulator, 3, 3), enter(emulator, 2, 3)]
        report = {'binary_sha256': hashlib.sha256((ROOT/'bin/enigma').read_bytes()).hexdigest(),
                  'emulator_sha256': hashlib.sha256(Path(shutil.which('zeal-native')).read_bytes()).hexdigest(),
                  'cpu_hz': CPU_HZ, 'nominal_video_tstates': VIDEO_TSTATES,
                  'method': 'Cycle timestamps at breakpoints in unmodified Z80 binary; controlled valid actor placement, no host-wall-time FPS.',
                  'room_entries': entries, 'scenarios': results}
        (ROOT/'build/profile-gameplay.json').write_text(json.dumps(report, indent=2)+'\n')
    finally:
        emulator.close()


if __name__ == '__main__':
    run()

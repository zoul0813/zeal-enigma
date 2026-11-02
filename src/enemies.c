#include <zgdk/sprites.h>
#include <zgdk/tilemap.h>
#include <zgdk/utils/rand.h>
#include "assets.h"
#include "enemies.h"
#include "player.h"

/* Original C16 tables at $2380 and $2340. TMX reverses world Y, so
 * translate the room index before looking up these packed difficulty values.
 */
static const uint8_t room_population[64] = {
    0x58, 0x58, 0x58, 0x58, 0x6a, 0x47, 0x59, 0x5a,
    0x4c, 0x5a, 0x4a, 0x5a, 0x6c, 0x5a, 0x6a, 0x46,
    0x57, 0x4c, 0x36, 0x48, 0x5a, 0x79, 0x57, 0x57,
    0x24, 0x58, 0x49, 0x47, 0x59, 0x68, 0x6f, 0x49,
    0x57, 0x6a, 0x4b, 0x11, 0x45, 0x58, 0x47, 0x6b,
    0x7e, 0x47, 0x47, 0x24, 0x68, 0xdf, 0xff, 0xac,
    0x46, 0x46, 0x35, 0x46, 0x35, 0x8e, 0xaf, 0xcf,
    0x69, 0x69, 0x58, 0x68, 0x8b, 0xbf, 0xff, 0xff,
};

static const uint8_t room_speed[64] = {
    0x56, 0x9a, 0x69, 0x69, 0x56, 0xbf, 0x56, 0x5a,
    0x66, 0x7b, 0x9a, 0x7b, 0x69, 0xdf, 0x7b, 0x9a,
    0x9a, 0x5a, 0xaa, 0x9a, 0xaf, 0x69, 0x6a, 0x7b,
    0x69, 0x9a, 0x56, 0x6a, 0x69, 0x59, 0x5a, 0x6a,
    0x5f, 0x59, 0x55, 0x6a, 0x7f, 0x9a, 0x56, 0x69,
    0x9a, 0xde, 0x7b, 0xaa, 0x6a, 0x69, 0x77, 0x5a,
    0x5a, 0x6a, 0x6b, 0x99, 0xa6, 0xaf, 0xaf, 0xff,
    0x5a, 0x67, 0x5a, 0x5a, 0x7f, 0xaf, 0xff, 0xff,
};

enemy_t ENEMIES[MAX_ENEMIES];
static uint8_t enemy_count;
static uint8_t enemy_delay;
static uint8_t enemy_tick;
static uint8_t speed_x, speed_y;

error enemies_init(void) {
    gfx_sprite sprite = { .tile = ENEMY_INACTIVE, .flags = SPRITE_NONE };
    for (uint8_t i = 0; i < MAX_ENEMIES; i++) {
        enemy_t *enemy = &ENEMIES[i];
        enemy->sprite_l = sprites_register(sprite);
        enemy->sprite_r = sprites_register(sprite);
        rect_set_pointer(&enemy->rect, 0, 0, SPRITE_WIDTH, SPRITE_HEIGHT);
    }
    enemies_clear();
    return GFX_SUCCESS;
}

void enemies_clear(void) {
    enemy_count = 0;
    for (uint8_t i = 0; i < MAX_ENEMIES; i++) enemy_destroy(&ENEMIES[i]);
}

void enemies_spawn(uint8_t room_id) {
    enemies_clear();
    if (room_id >= WORLD_WIDTH * WORLD_HEIGHT) return;
    uint8_t original = (7 - room_id / WORLD_WIDTH) * WORLD_WIDTH + room_id % WORLD_WIDTH;
    uint8_t population = room_population[original];
    if (game_difficulty <= 2) population &= 15;
    else population >>= 4;
    uint8_t requested = population + 1;
    uint8_t speed = room_speed[original];
    if (game_difficulty & 1) speed >>= 4;
    else speed &= 15;
    // Original multicolor X units are twice as wide as Y pixels; our art is 2x.
    speed_x = ((speed >> 2) & 3) * 4;
    speed_y = (speed & 3) * 2;
    enemy_delay = (5 - game_difficulty) * 16 + 1;
    enemy_tick = 0;

    Vector2_u8 *spawns = (Vector2_u8 *)data;
    uint16_t count = 0;
    uint8_t player_col = (player.point.x - SPRITE_WIDTH) / SPRITE_WIDTH;
    uint8_t player_row = (player.point.y - SPRITE_HEIGHT - ROOM_TOP_PX) / SPRITE_HEIGHT;
    for (uint8_t row = 0; row < ROOM_HEIGHT; row++) {
        for (uint8_t col = 0; col + 1 < ROOM_WIDTH; col++) {
            // The original excludes the player's 8x7-character map block.
            if (col / 8 == player_col / 8 && row / 7 == player_row / 7) continue;
            if (tilemap_get_xy(col, row) != TILE_NONE || tilemap_get_xy(col + 1, row) != TILE_NONE) continue;
            spawns[count].x = col;
            spawns[count++].y = row;
        }
    }
    while (enemy_count < requested && count) {
        uint16_t random = ((uint16_t)(uint8_t)rand8() << 8) | (uint8_t)rand8();
        Vector2_u8 spawn;
        spawn = spawns[random % count];
        enemy_t *enemy = &ENEMIES[enemy_count++];
        enemy->type = (uint8_t)rand8() & 7;
        enemy->sprite_l->tile = ENEMY_INDEX + enemy->type * 4;
        enemy->sprite_r->tile = enemy->sprite_l->tile + 1;
        enemy_set_xy(enemy, (spawn.x + ROOM_OFFSET_X + 1) * SPRITE_WIDTH,
            (spawn.y + ROOM_OFFSET_Y + 1) * SPRITE_HEIGHT);
        uint16_t remaining = 0;
        for (uint16_t j = 0; j < count; j++) {
            Vector2_u8 candidate;
            candidate = spawns[j];
            if (candidate.y == spawn.y && candidate.x + 1 >= spawn.x && candidate.x <= spawn.x + 1) continue;
            spawns[remaining++] = candidate;
        }
        count = remaining;
    }
}

/* Check the entire two-tile footprint in the logical room map. Axis moves are
 * independent, so an enemy can slide along a wall, but cannot cross it. */
static bool can_stand(int16_t x, int16_t y) {
    if (x < (int16_t)(ROOM_LEFT_PX + SPRITE_WIDTH) || x + (int16_t)SPRITE_WIDTH > ROOM_RIGHT_PX ||
        y < (int16_t)(ROOM_TOP_PX + SPRITE_HEIGHT) || y > (int16_t)ROOM_BOTTOM_PX) return false;
    uint8_t left = (x - SPRITE_WIDTH) / SPRITE_WIDTH - ROOM_OFFSET_X;
    uint8_t right = (x + SPRITE_WIDTH - 1) / SPRITE_WIDTH - ROOM_OFFSET_X;
    uint8_t top = (y - SPRITE_HEIGHT) / SPRITE_HEIGHT - ROOM_OFFSET_Y;
    uint8_t bottom = (y - 1) / SPRITE_HEIGHT - ROOM_OFFSET_Y;
    for (uint8_t row = top; row <= bottom; row++)
        for (uint8_t col = left; col <= right; col++)
            if (tilemap_get_xy(col, row) != TILE_NONE) return false;
    return true;
}

void enemies_move(Point *target) {
    enemy_tick++;
    bool waiting = enemy_delay != 0;
    if (waiting) enemy_delay--;
    if (enemy_tick & 1) return;
    for (uint8_t i = 0; i < enemy_count; i++) {
        enemy_t *enemy = &ENEMIES[i];
        if (enemy->sprite_l->tile == ENEMY_INACTIVE) continue;
        enemy->sprite_l->tile = ENEMY_INDEX + enemy->type * 4 + ((enemy_tick >> 3) & 1) * 2;
        enemy->sprite_r->tile = enemy->sprite_l->tile + 1;
        if (waiting) continue;
        int16_t x = enemy->point.x, y = enemy->point.y;
        // Original pursuit compares coarse X/Y buckets rather than pathfinding.
        int16_t dx = (target->x - SPRITE_WIDTH) >> 4;
        int16_t ex = (x - SPRITE_WIDTH) >> 4;
        int16_t dy = (target->y - SPRITE_HEIGHT) >> 3;
        int16_t ey = (y - SPRITE_HEIGHT) >> 3;
        int16_t next = x + ((dx > ex) - (dx < ex)) * speed_x;
        if (can_stand(next, y)) x = next;
        next = y + ((dy > ey) - (dy < ey)) * speed_y;
        if (can_stand(x, next)) y = next;
        enemy_set_xy(enemy, x, y);
    }
}

bool enemies_touch_player(const Point *point) {
    for (uint8_t i = 0; i < enemy_count; i++) {
        const enemy_t *enemy = &ENEMIES[i];
        if (enemy->sprite_l->tile == ENEMY_INACTIVE) continue;
        // Sprite coordinates are bottom-right. Player: one column, two rows;
        // enemy: two columns, one row. Edge contact without overlap is safe.
        if (point->x > enemy->point.x - SPRITE_WIDTH && point->x - SPRITE_WIDTH < enemy->point.x + SPRITE_WIDTH &&
            point->y + SPRITE_HEIGHT > enemy->point.y - SPRITE_HEIGHT && point->y - SPRITE_HEIGHT < enemy->point.y)
            return true;
    }
    return false;
}

uint16_t enemy_score(const enemy_t *enemy) {
    static const uint16_t values[8] = { 10, 20, 40, 80, 100, 200, 400, 800 };
    return values[enemy->type & 7];
}

void enemy_destroy(enemy_t *enemy) {
    enemy->sprite_l->tile = ENEMY_INACTIVE;
    enemy->sprite_r->tile = ENEMY_INACTIVE;
    enemy->sprite_l->x = SCREEN_WIDTH + SPRITE_WIDTH;
    enemy->sprite_r->x = SCREEN_WIDTH + SPRITE_WIDTH;
}

void enemy_set_xy(enemy_t *enemy, uint16_t x, uint16_t y) {
    vec2_set_pointer(&enemy->point, x, y);
    sprite_set_xy(enemy->sprite_l, x, y);
    sprite_set_xy(enemy->sprite_r, x + SPRITE_WIDTH, y);
}

inline void enemy_set_point(enemy_t *enemy, Point *point) {
    enemy_set_xy(enemy, point->x, point->y);
}

#include <stdio.h>
#include <zgdk/utils/print.h>

#include <zgdk/types.h>
#include <zgdk/sprites.h>
#include <zgdk/tiles.h>
#include <zgdk/tilemap.h>
#include <zgdk/utils/rand.h>

#include "assets.h"
#include "enemies.h"
#include "world.h"

enemy_t ENEMIES[MAX_ENEMIES];

uint8_t ENEMY_ROOMS[WORLD_WIDTH * WORLD_HEIGHT] = {
    3, 4, 3, 4, 3, 2, 4, 5,
    4, 3, 4, 3, 5, 3, 6, 4,
    3, 4, 3, 2, 4, 7, 3, 4,
    3, 5, 3, 2, 4, 3, 8, 4,
    3, 4, 6, 3, 4, 5, 3, 2,
    4, 3, 7, 4, 3, 4, 3, 5,
    3, 4, 3, 6, 4, 3, 4, 2,
    3, 5, 4, 3, 4, 8, 3, 4
};

error enemies_init(void)
{
    gfx_sprite sprite = {
        .tile = ENEMY_INACTIVE,
        .x = SCREEN_WIDTH + SPRITE_HEIGHT,
        .y = SCREEN_HEIGHT / 2,
    };

    uint8_t i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        enemy_t* enemy   = &ENEMIES[i];
        enemy->sprite_l  = sprites_register(sprite);
        enemy->sprite_r  = sprites_register(sprite);

        vec2_set(enemy->direction, DIRECTION_NONE, DIRECTION_NONE);
        rect_set_pointer(&enemy->rect, 0, 0, SPRITE_WIDTH, SPRITE_HEIGHT);
        enemy_set_xy(enemy, WORLD_PLAYER_X, WORLD_PLAYER_Y);
        vec2_copy_pointer(&enemy->last_point, &enemy->point);
    }

    return GFX_SUCCESS;
}

static uint8_t enemy_count = 0;
void enemies_spawn(uint8_t roomId)
{
    if(roomId >= sizeof(ENEMY_ROOMS)) return;
    enemy_count = ENEMY_ROOMS[roomId];

    Vector2_u8* validSpawn = (Vector2_u8*)data;  // Use flat array

    uint16_t spawns = 0;
    for(uint8_t row = 0; row < ROOM_HEIGHT; row++) {
        for(uint8_t col = 0; col + 1 < ROOM_WIDTH; col++) {
            if(tilemap_get_xy(col, row) == TILE_NONE && tilemap_get_xy(col + 1, row) == TILE_NONE) {
                validSpawn[spawns].x = col + ROOM_OFFSET_X;
                validSpawn[spawns].y = row + ROOM_OFFSET_Y;
                spawns++;
            }
        }
    }

    uint8_t i;
    for(i = 0; i < enemy_count && spawns > 0; i++) {
        enemy_t* enemy = &ENEMIES[i];
        uint16_t r = (uint8_t)rand8();
        r = (r << 8) | (uint8_t)rand8();
        r %= spawns;
        Vector2_u8 spawn;
        vec2_copy(spawn, validSpawn[r]);
        Tile tile_l;
        tile_at(spawn.x, spawn.y, &tile_l);

        r &= 0x07;
        if(r > 4) r -= 4;
        r *= 4;
        enemy->sprite_l->tile = ENEMY_INDEX + r;
        enemy->sprite_r->tile = ENEMY_INDEX + r + 1;

        enemy_set_point(enemy, &tile_l.rect.point);

        // Remove every candidate whose two-tile footprint overlaps this enemy.
        uint16_t remaining = 0;
        for(uint16_t j = 0; j < spawns; j++) {
            Vector2_u8 candidate;
            vec2_copy(candidate, validSpawn[j]);
            if(candidate.y == spawn.y && candidate.x + 1 >= spawn.x && candidate.x <= spawn.x + 1)
                continue;
            validSpawn[remaining++] = candidate;
        }
        spawns = remaining;
    }

    enemy_count = i;
    for (i = enemy_count; i < MAX_ENEMIES; i++) {
        enemy_t* enemy        = &ENEMIES[i];
        enemy_destroy(enemy);
    }
}

static uint8_t enemy_last_moved = 0;
void enemies_move(Point *player_point) {
    enemy_last_moved++;
    if(enemy_last_moved >= enemy_count) enemy_last_moved = 0;

    uint8_t i;
    enemy_t *enemy = NULL;
    for(i = enemy_last_moved; i < enemy_count; i++) {
        if(ENEMIES[i].sprite_l->tile != ENEMY_INACTIVE) {
            enemy = &ENEMIES[i];
            break;
        }
    }

    enemy_last_moved = i;
    if(enemy == NULL) return;

    vec2_direction(&enemy->direction, player_point, &enemy->point);
    vec2_copy_pointer(&enemy->last_point, &enemy->point);

    // can't move
    if(enemy->direction.x == 0 && enemy->direction.y == 0) return;

    uint16_t x  = enemy->point.x;
    uint16_t y  = enemy->point.y;
    x          += (enemy->direction.x * ENEMY_SPEED);
    y          += (enemy->direction.y * ENEMY_SPEED);

    if (x < SPRITE_WIDTH)
        x = SPRITE_WIDTH;
    else if (x > SCREEN_WIDTH)
        x = SCREEN_WIDTH;
    if (y < SPRITE_HEIGHT)
        y = SPRITE_HEIGHT;
    else if (y > SCREEN_HEIGHT)
        y = SCREEN_HEIGHT;

    // didn't move
    if((x == enemy->point.x) && (y == enemy->point.y)) return;

    enemy_set_xy(enemy, x, y);
    enemy_collide(enemy);
}

Edge enemy_collide(enemy_t *enemy) {
    Tile left_edge_tile, right_edge_tile;
    int16_t left_tile, right_tile;
    Edge edge = EdgeNone;

    Rect left_rect, right_rect;
    rect_set_pointer(&left_rect, enemy->point.x, enemy->point.y, enemy->size.w, enemy->size.h);
    rect_set_pointer(&right_rect, enemy->point.x, enemy->point.y, enemy->size.w, enemy->size.h);
    right_rect.x += SPRITE_WIDTH;

    if(enemy->direction.x < 0) {
        if(tile_get_edge(&left_rect, EdgeLeft, &left_edge_tile)) {
            left_tile = tilemap_get(&left_edge_tile.point);
            if((left_tile < 0)) {
                return EdgeLeft;
            } else if((left_tile != 0)) {
                enemy_set_xy(enemy, left_edge_tile.rect.x, enemy->point.y);
            }
        } else {
            return EdgeLeft;
        }
    } else if(enemy->direction.x > 0) {
        if(tile_get_edge(&right_rect, EdgeRight, &right_edge_tile)) {
            right_tile = tilemap_get(&right_edge_tile.point);
            if((right_tile < 0)) {
                return EdgeRight;
            } else if((right_tile != 0)) {
                enemy_set_xy(enemy, right_edge_tile.rect.x - SPRITE_WIDTH, enemy->point.y);
            }
        }
    }

    if(enemy->direction.y < 0) {
        if(tile_get_edge(&left_rect, EdgeTop, &left_edge_tile) && tile_get_edge(&right_rect, EdgeTop, &right_edge_tile)) {
            left_tile = tilemap_get(&left_edge_tile.point);
            right_tile = tilemap_get(&right_edge_tile.point);
            if((left_tile < 0) | (right_tile < 0)) {
                return EdgeTop;
            } else if((left_tile != TILE_NONE) || (right_tile != TILE_NONE)) {
                enemy_set_xy(enemy, enemy->point.x, left_edge_tile.rect.y);
            }
        }
    } else if(enemy->direction.y > 0) {
        if(tile_get_edge(&left_rect, EdgeBottom, &left_edge_tile) && tile_get_edge(&right_rect, EdgeBottom, &right_edge_tile)) {
            left_tile = tilemap_get(&left_edge_tile.point);
            right_tile = tilemap_get(&right_edge_tile.point);
            if((left_tile < 0) | (right_tile < 0)) {
                return EdgeBottom;
            } else if((left_tile != TILE_NONE) || (right_tile != TILE_NONE)) {
                enemy_set_xy(enemy, enemy->point.x, left_edge_tile.rect.y);
            }
        }
    }

    return EdgeNone;
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

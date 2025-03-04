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
        enemy->direction.x = DIRECTION_LEFT;
        enemy->direction.y = DIRECTION_DOWN;
    }

    return GFX_SUCCESS;
}

void enemies_spawn(uint8_t roomId)
{
    if(roomId >= sizeof(ENEMY_ROOMS)) return;

    uint8_t count = ENEMY_ROOMS[roomId];
    Vector2_u8* validSpawn = (Vector2_u8*)data;  // Use flat array

    uint16_t spawns = 0;
    for(uint8_t row = 0; row < ROOM_HEIGHT; row++) {
        for(uint8_t col = 0; col < ROOM_WIDTH; col++) {
            uint8_t tile = tilemap_get_xy(row, col);
            if(tile == TILE_NONE) {
                validSpawn[spawns].x = col;
                validSpawn[spawns].y = row;
                spawns++;
            }
        }
    }

    uint8_t i, r;
    for(i = 0; i < count; i++) {
        enemy_t* enemy = &ENEMIES[i];
        r = rand8() % spawns;
        while(validSpawn[r].x != 0xFF) {
            r++;
            if(r >= spawns) r = 0;
        }

        Tile tile;
        tile_at(validSpawn[r].x, validSpawn[r].y, &tile);

        validSpawn[r].x = 0xFF; // mark the spot as occupied

        r &= 0x07;
        if(r > 4) r -= 4;
        enemy->sprite_l->tile = r;
        enemy->sprite_r->tile = r + 1;

        sprite_set_xy(enemy->sprite_l, tile.rect.point.x, tile.rect.point.y);
        sprite_set_xy(enemy->sprite_r, tile.rect.point.x + SPRITE_WIDTH, tile.rect.point.y);
    }

    for (i = count; i < MAX_ENEMIES; i++) {
        enemy_t* enemy        = &ENEMIES[i];
        enemy_destroy(enemy);
    }
}

void enemies_move(void) {
    uint8_t i;

    uint16_t x, y;
    uint8_t tx, ty;
    for(i = 0; i < MAX_ENEMIES; i++) {
        enemy_t *enemy = &ENEMIES[i];
        if(enemy->sprite_l->tile == ENEMY_INACTIVE) continue;

        x = enemy->sprite_l->x;
        y = enemy->sprite_l->y;

        x += enemy->direction.x * ENEMY_SPEED;
        y += enemy->direction.y * ENEMY_SPEED;

        if(x < ROOM_LEFT_PX + SPRITE_WIDTH) {
            x = ROOM_LEFT_PX + SPRITE_WIDTH + 1;
            enemy->direction.x *= -1;
        }
        if(x > ROOM_RIGHT_PX - SPRITE_WIDTH) {
            x = ROOM_RIGHT_PX - SPRITE_WIDTH - 1;
            enemy->direction.x *= -1;
        }

        if(y < ROOM_TOP_PX + SPRITE_HEIGHT) {
            y = ROOM_TOP_PX + SPRITE_HEIGHT + 1;
            enemy->direction.y *= -1;
        }

        if(y > ROOM_BOTTOM_PX - SPRITE_WIDTH) {
            y = ROOM_BOTTOM_PX - SPRITE_WIDTH - 1;
            enemy->direction.y *= -1;
        }

        tx = ((x + 8) >> 4) - 1;
        if(enemy->direction.x > 0)
            tx++;
        ty = ((y + 8) >> 4) - 1;
        if(enemy->direction.y > 0)
            ty++;
        TileType type = tilemap_get_xy(tx, ty);
        if(type != TILE_NONE) {
            enemy->direction.x *= -1;
            enemy->direction.y *= -1;
        }

        sprite_set_xy(enemy->sprite_l, x, y);
        sprite_set_xy(enemy->sprite_r, x + SPRITE_WIDTH, y);
    }
}

void enemy_destroy(enemy_t *enemy) {
    enemy->sprite_l->tile = ENEMY_INACTIVE;
    enemy->sprite_r->tile = ENEMY_INACTIVE;
    enemy->sprite_l->x = SCREEN_WIDTH + SPRITE_WIDTH;
    enemy->sprite_r->x = SCREEN_WIDTH + SPRITE_WIDTH;
}

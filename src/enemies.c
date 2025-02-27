#include <stdio.h>
#include <zgdk/utils/print.h>

#include <zgdk/types.h>
#include <zgdk/sprites.h>
#include <zgdk/utils/rand.h>

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

    uint8_t i, r1, r2, r, offset;
    uint8_t x, y;
    uint16_t px, py;
    TileType type;
    for (i = 0; i < count; i++) {
        enemy_t* enemy = &ENEMIES[i];

        do {
            // get two random nunmbers
            r1 = rand8();
            r2 = rand8();

            // low + high nibble, convert to 5-bit
            r = ((r1 & 0x0F) | (r2 & 0xF0)) >> 3; // 0..31 (ROOM_WIDTH - 8)
            x = r;
            // offset by 4 tiles on either side (8)
            x += ROOM_OFFSET_X + 4;

            // low + high nibble, convert to 5-bit
            r = ((r2 & 0x0F) | (r1 & 0xF0)) >> 3; // 0..31
            // limit the value between the boundaries
            y = (r % (ROOM_HEIGHT - 8));
            // offset by 2 tiles on either side (4)
            y += ROOM_OFFSET_Y + 4;

            type = screen_get_tile(x, y);
        } while (type != TILE_NONE);

        sprintf(buffer, "%03d %03d %02d", x, y, type);
        nprint_string_layer(&vctx, buffer, 10, LAYER0, WIDTH - 10, HEIGHT - 1);

        px = x * SPRITE_WIDTH;
        py = y * SPRITE_HEIGHT;


        offset = (((r1 & 0x0F) | (r2 >> 4))) & 0x03;
        if(r1 > 127) offset++;

        offset = (offset * 2) + ENEMY_INDEX;

        enemy->sprite_l->tile = offset;
        enemy->sprite_r->tile = offset + 1;
        enemy->sprite_l->x    = px;
        enemy->sprite_l->y    = py;
        enemy->sprite_r->x    = px + SPRITE_WIDTH;
        enemy->sprite_r->y    = py;
    }

    for (i = count; i < MAX_ENEMIES; i++) {
        enemy_t* enemy        = &ENEMIES[i];
        enemy->sprite_l->tile = ENEMY_INACTIVE;
        enemy->sprite_r->tile = ENEMY_INACTIVE;
        enemy->sprite_l->x    = SCREEN_WIDTH + SPRITE_WIDTH;
        enemy->sprite_r->x    = SCREEN_WIDTH + SPRITE_WIDTH;
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

        x *= enemy->direction.x;
        y *= enemy->direction.y;

        tx = (x >> 4) + 1;
        ty = (y >> 4) + 1;
        TileType type = screen_get_tile(x, y);
        if(type != TILE_NONE) {
            enemy->direction.x *= -1;
            enemy->direction.y *= -1;
        }

        enemy->sprite_l->x = x;
        enemy->sprite_l->y = y;
        enemy->sprite_r->x = x + SPRITE_WIDTH;
        enemy->sprite_r->y = y;

    }
}

void enemy_destroy(enemy_t *enemy) {
    enemy->sprite_l->tile = ENEMY_INACTIVE;
    enemy->sprite_r->tile = ENEMY_INACTIVE;
    enemy->sprite_l->x = SCREEN_WIDTH + SPRITE_WIDTH;
    enemy->sprite_r->x = SCREEN_WIDTH + SPRITE_WIDTH;
}

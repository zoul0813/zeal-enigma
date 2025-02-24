#include <stdio.h>
#include <string.h>
#include <zos_errors.h>
#include <zvb_gfx.h>
#include <zvb_sprite.h>

#include "player.h"
#include "main.h"
#include "world.h"

#define PLAYER_TILE 16

player_t player;

error player_init(void)
{
    player.score = 0;
    player.point.x = WORLD_PLAYER_X;
    player.point.y = WORLD_PLAYER_Y;

    gfx_sprite sprite = {
        .tile = PLAYER_TILE,
        .flags = SPRITE_NONE,
    };
    player.sprite_head = sprites_register(sprite);

    sprite.tile += 9; // body is always header + 9
    player.sprite_body = sprites_register(sprite);

    return GFX_SUCCESS;
}

error player_deinit(void)
{
    return ERR_SUCCESS;
}

Edge player_move(void)
{
    uint16_t x = player.point.x;
    uint16_t y = player.point.y;
    x          += (player.direction.x * PLAYER_SPEED);
    y          += (player.direction.y * PLAYER_SPEED);

    switch (player.direction.x) {
        case DIRECTION_LEFT: {
            player.sprite_head->flags = SPRITE_FLIP_X;
            player.sprite_body->flags = SPRITE_FLIP_X;
        } break;
        case DIRECTION_RIGHT: {
            player.sprite_head->flags = SPRITE_NONE;
            player.sprite_body->flags = SPRITE_NONE;
        } break;
    }

    // TODO: better tile x/y coordinate system?
    uint8_t tx, ty;
    tx = (x >> 4) - 1;
    if(player.direction.x > 0) tx++;
    ty = (y >> 4) - 1;
    if(player.direction.y > 0) ty++;

    TileType type = screen_get_tile(tx, ty);

    switch(type) {
        case TILE_UNKNOWN: // fall thru
        case TILE_WALL:
            // TODO: only block movement on the direction that has the block
            // obstruction
            return EdgeNone;
        // case TILE_CHEST:
        //     player.score++;
        //     world_set_tile(tx, ty, 0); // make it a ground tile

        //     // player score
        //     sprintf(buffer, "%03d", player.score);
        //     nprint_string(&vctx, buffer, 3, WIDTH-3, HEIGHT-1);
        //     break;
    }


    player_set_xy(x, y);

    if(x < SPRITE_WIDTH) return EdgeLeft;
    else if(x > SCREEN_WIDTH) return EdgeRight;
    if(y < SPRITE_HEIGHT) return EdgeTop;
    else if(y > SCREEN_HEIGHT) return EdgeBottom;
    return EdgeNone;
}

void player_set_xy(uint16_t x, uint16_t y) {
    player.point.x = x;
    player.point.y = y;
    player.sprite_body->x = x;
    player.sprite_head->x = x;
    player.sprite_body->y = y;
    player.sprite_head->y = y - SPRITE_HEIGHT;
}

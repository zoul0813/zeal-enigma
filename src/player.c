#include <zos_errors.h>
#include <zvb_gfx.h>
#include <zvb_sprite.h>
#include "player.h"
#include "main.h"

player_t player;

error player_init(void)
{
    gfx_sprite sprite = {
        .tile = 2,
        .flags = SPRITE_NONE,
    };

    player.point.x     = SCREEN_WIDTH / 2;
    player.point.y     = SCREEN_HEIGHT / 2;

    player.sprite_body = sprites_register(sprite);

    sprite.tile = 1;
    player.sprite_body = sprites_register(sprite);

    return ERR_SUCCESS;
}

error player_deinit(void)
{
    return ERR_SUCCESS;
}

void player_move(void)
{
    uint16_t x  = player.point.x;
    uint16_t y  = player.point.y;
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

    if(x < SPRITE_WIDTH) x = SPRITE_WIDTH;
    else if(x > SCREEN_WIDTH) x = SCREEN_WIDTH;
    if(y < SPRITE_HEIGHT) y = SPRITE_HEIGHT;
    else if(y > SCREEN_HEIGHT + SPRITE_HEIGHT) y = SCREEN_HEIGHT + SPRITE_HEIGHT;

    player_set_xy(x, y);
}

void player_set_xy(uint16_t x, uint16_t y) {
    player.point.x = x;
    player.point.y = y;
    player.sprite_body->x = x;
    player.sprite_head->x = x;
    player.sprite_body->y = y;
    player.sprite_head->y = y - SPRITE_HEIGHT;
}

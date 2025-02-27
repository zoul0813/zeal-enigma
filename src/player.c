#include <zos_errors.h>
#include <zvb_gfx.h>
#include <zvb_sprite.h>
#include <zgdk/sprites.h>

#include "player.h"
#include "bullet.h"
#include "main.h"
#include "world.h"
#include "objects.h"
#include "hud.h"

#define PLAYER_TILE 16

#define BODY_TILE() (player.sprite_head->tile + 9)

player_t player;

static uint8_t frame        = 0;
static uint8_t frame_offset = 1;
static int8_t frame_dir     = 1;

error player_init(void)
{
    player.score  = 0;
    player.energy = MAX_ENERGY;
    player.lives  = 3;
    player.keys   = 0;

    player.point.x = WORLD_PLAYER_X;
    player.point.y = WORLD_PLAYER_Y;

    gfx_sprite sprite = {
        .tile  = PLAYER_TILE + frame_offset,
        .flags = SPRITE_NONE,
    };
    player.sprite_head = sprites_register(sprite);

    sprite.tile        = BODY_TILE(); // body is always header + 9
    player.sprite_body = sprites_register(sprite);

    return GFX_SUCCESS;
}

error player_deinit(void)
{
    return ERR_SUCCESS;
}

Edge player_move(void)
{
    if (frame > 7) {
        frame         = 0;
        frame_offset += frame_dir;
        if (frame_offset == 0 || frame_offset == 2) {
            frame_dir *= -1;
        }
    }

    uint16_t x  = player.point.x;
    uint16_t y  = player.point.y;
    x          += (player.direction.x * PLAYER_SPEED);
    y          += (player.direction.y * PLAYER_SPEED);

    Edge edge = EdgeNone;
    switch (player.direction.x) {
        case DIRECTION_LEFT:
            player.sprite_head->tile  = PLAYER_TILE + 6 + frame_offset;
            player.sprite_body->tile  = BODY_TILE();
            player.sprite_head->flags = SPRITE_FLIP_X;
            player.sprite_body->flags = SPRITE_FLIP_X;
            edge                      = EdgeLeft;
            break;
        case DIRECTION_RIGHT:
            player.sprite_head->tile  = PLAYER_TILE + 6 + frame_offset;
            player.sprite_body->tile  = BODY_TILE();
            player.sprite_head->flags = SPRITE_NONE;
            player.sprite_body->flags = SPRITE_NONE;
            edge                      = EdgeRight;
            break;
    }

    switch (player.direction.y) {
        case DIRECTION_UP:
            player.sprite_head->tile = PLAYER_TILE + frame_offset;
            player.sprite_body->tile = BODY_TILE();
            edge                     = EdgeTop;
            break;
        case DIRECTION_DOWN:
            player.sprite_head->tile = PLAYER_TILE + 3 + frame_offset;
            player.sprite_body->tile = BODY_TILE();
            edge                     = EdgeBottom;
            break;
    }

    // TODO: better tile x/y coordinate system?
    uint8_t tx, ty;
    tx = ((x) >> 4) - 1;
    if (player.direction.x > 0)
        tx++;
    ty = ((y) >> 4) - 1;
    if (player.direction.y > 0)
        ty++;

    TileType type_head = room_get_tile(tx - ROOM_OFFSET_X, ty - ROOM_OFFSET_Y);
    TileType type_body = room_get_tile(tx - ROOM_OFFSET_X, ty - ROOM_OFFSET_Y + 1);

    // Point point         = {
    //     .x = player.point.x + 8,
    //     .y = player.point.y + 8,
    // };
    // TileType type_head  = screen_get_tile(&point, edge);
    // point.y            += SPRITE_HEIGHT;
    // TileType type_body  = screen_get_tile(&point, edge);

    // check the head first
    TileType type = type_head;
    // if it's empty, try the body?
    if(type == TILE_NONE) type = type_body;

#if DEBUG
    // type
    sprintf(buffer, "%03d", type);
    nprint_string_layer(&vctx, buffer, 3, LAYER0, 0, HEIGHT - 3);

    // player x,y
    sprintf(buffer, "%03d %03d", player.point.x, player.point.y);
    nprint_string_layer(&vctx, buffer, 7, LAYER0, 0, HEIGHT - 2);

    // screen x,y
    sprintf(buffer, "%03d %03d", tx, ty);
    nprint_string_layer(&vctx, buffer, 7, LAYER0, 0, HEIGHT - 1);
#endif

    frame++;

    switch (type) {
        case TILE_UNKNOWN: // fall thru
        case TILE_WALL:
            // TODO: only block movement on the direction that has the block
            // obstruction
            return EdgeNone;
    }

    if ((type >= KEYS_INDEX) && (type <= KEYS_INDEX + keys_len)) {
        uint8_t value = type - KEYS_INDEX;
        object_t* key = &keys[value];
        key->flags    = OBJECT_INACTIVE;
        room_set_tile(key->x, key->y, TILE_NONE); // make it a ground tile

        player.score += 5000; /// 5,000 for keys
        player.keys  |= (1 << value);

        for (uint8_t i = 0; i < gates_len; i++) {
            object_t* gate = &gates[i];
            if (gate->id == value) {
                gate->flags = OBJECT_INACTIVE;
            }
        }

        hud_score(player.score);
        hud_keys(player.keys);
    }

    if ((type >= GATES_INDEX) && (type <= GATES_INDEX + gates_len)) {
        return EdgeNone;
    }

    uint8_t roomId = ROOM_ID();

    if (type == FUEL_INDEX) {
        object_t* can;
        for (uint8_t i = 0; i < fuel_len; i++) {
            can = &fuel[i];
            if (can->room == roomId) {
                can->flags = OBJECT_INACTIVE;
                room_set_tile(can->x, can->y, TILE_NONE); // make it a ground tile

                player.energy += FUEL_VALUE;
                if (player.energy > MAX_ENERGY)
                    player.energy = MAX_ENERGY;

                hud_energy(player.energy);
                break;
            }
        }
    }


    player_set_xy(x, y);

    if (x < (ROOM_LEFT_PX + SPRITE_WIDTH))
        return EdgeLeft;
    else if (x > (ROOM_RIGHT_PX))
        return EdgeRight;
    if (y < (ROOM_TOP_PX + SPRITE_HEIGHT))
        return EdgeTop;
    else if (y > (ROOM_BOTTOM_PX))
        return EdgeBottom;
    return EdgeNone;
}

void player_set_xy(uint16_t x, uint16_t y)
{
    player.point.x        = x;
    player.point.y        = y;
    player.sprite_head->x = x;
    player.sprite_head->y = y;
    player.sprite_body->x = x;
    player.sprite_body->y = y + SPRITE_HEIGHT;
}

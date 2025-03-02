#include <zos_errors.h>
#include <zvb_gfx.h>
#include <zvb_sprite.h>
#include <zgdk/sprites.h>
#include <zgdk/tilemap.h>
#include <zgdk/tiles.h>

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

    player.direction.x = 0;
    player.direction.y = 0;

    gfx_sprite sprite = {
        .tile  = PLAYER_TILE + frame_offset,
        .flags = SPRITE_NONE,
    };
    player.sprite_head = sprites_register(sprite);

    sprite.tile        = BODY_TILE(); // body is always header + 9
    player.sprite_body = sprites_register(sprite);

    player_set_xy(WORLD_PLAYER_X, WORLD_PLAYER_Y);
    point_copy(&player.last_point, &player.point);

    return GFX_SUCCESS;
}

error player_deinit(void)
{
    return ERR_SUCCESS;
}

void player_animate(void) {
    if (frame > 7) {
        frame         = 0;
        frame_offset += frame_dir;
        if (frame_offset == 0 || frame_offset == 2) {
            frame_dir *= -1;
        }
    }

    switch (player.direction.x) {
        case DIRECTION_LEFT:
            player.sprite_head->tile  = PLAYER_TILE + 6 + frame_offset;
            player.sprite_body->tile  = BODY_TILE();
            player.sprite_head->flags = SPRITE_FLIP_X;
            player.sprite_body->flags = SPRITE_FLIP_X;
            break;
        case DIRECTION_RIGHT:
            player.sprite_head->tile  = PLAYER_TILE + 6 + frame_offset;
            player.sprite_body->tile  = BODY_TILE();
            player.sprite_head->flags = SPRITE_NONE;
            player.sprite_body->flags = SPRITE_NONE;
            break;
    }

    switch (player.direction.y) {
        case DIRECTION_UP:
            player.sprite_head->tile = PLAYER_TILE + frame_offset;
            player.sprite_body->tile = BODY_TILE();
            break;
        case DIRECTION_DOWN:
            player.sprite_head->tile = PLAYER_TILE + 3 + frame_offset;
            player.sprite_body->tile = BODY_TILE();
            break;
    }

    frame++;
}

bool player_move(void)
{
    point_copy(&player.last_point, &player.point);

    // can't move
    if(player.direction.x == 0 && player.direction.y == 0) return false;

    uint16_t x  = player.point.x;
    uint16_t y  = player.point.y;
    x          += (player.direction.x * PLAYER_SPEED);
    y          += (player.direction.y * PLAYER_SPEED);

    if (x < SPRITE_WIDTH)
        x = SPRITE_WIDTH;
    else if (x > SCREEN_WIDTH)
        x = SCREEN_WIDTH;
    if (y < SPRITE_HEIGHT)
        y = SPRITE_HEIGHT;
    else if (y > SCREEN_HEIGHT)
        y = SCREEN_HEIGHT;

    // didn't move
    if((x == player.point.x) && (y == player.point.y)) return false;

    player_set_xy(x, y);
    return true;
}

Edge player_collide(void) {
    Tile edge_tile;
    if(player.direction.x < 0) {
        if(tile_get_edge(&player.rect, EdgeLeft, &edge_tile)) {
            tilemap_place(&vctx, LAYER1, 2, &edge_tile.point);
            uint8_t tile = tilemap_get(&edge_tile.point);
            if(tile != TILE_NONE) {
                player_set_xy(edge_tile.rect.x, player.point.y);
            }
        }
    } else if(player.direction.x > 0) {
        if(tile_get_edge(&player.rect, EdgeRight, &edge_tile)) {
            tilemap_place(&vctx, LAYER1, 2, &edge_tile.point);
            uint8_t tile = tilemap_get(&edge_tile.point);
            if(tile != 0) {
                player_set_xy(edge_tile.rect.x, player.point.y);
            }
        }
    }

    if(player.direction.y < 0) {
        if(tile_get_edge(&player.rect, EdgeTop, &edge_tile)) {
            tilemap_place(&vctx, LAYER1, 2, &edge_tile.point);
            uint8_t tile = tilemap_get(&edge_tile.point);
            if(tile != TILE_NONE) {
                player_set_xy(player.point.x, edge_tile.rect.y);
            }
        }
    } else if(player.direction.y > 0) {
        if(tile_get_edge(&player.rect, EdgeBottom, &edge_tile)) {
            tilemap_place(&vctx, LAYER1, 2, &edge_tile.point);
            uint8_t tile = tilemap_get(&edge_tile.point);
            if(tile != TILE_NONE) {
                player_set_xy(player.point.x, edge_tile.rect.y);
            }
        }
    }

    return EdgeNone;

    uint16_t x  = player.point.x;
    uint16_t y  = player.point.y;
    Edge edge = EdgeNone;

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

    return edge;
}

void player_set_xy(uint16_t x, uint16_t y) {
    point_set(&player.point, x, y);
    sprite_set_xy(player.sprite_head, x, y);
    sprite_set_xy(player.sprite_body, x, y + SPRITE_HEIGHT);
}
inline void player_set_point(Point *point) {
    player_set_xy(point->x, point->y);
}

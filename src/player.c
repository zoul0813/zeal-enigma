#include <zos_errors.h>
#include <zvb_gfx.h>
#include <zvb_sprite.h>
#include <zgdk/sprites.h>
#include <zgdk/tilemap.h>
#include <zgdk/tiles.h>

#include <stdio.h>
#include <zgdk/utils/print.h>

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

    gfx_sprite sprite = {
        .tile  = PLAYER_TILE + frame_offset,
        .flags = SPRITE_NONE,
    };
    player.sprite_head = sprites_register(sprite);

    sprite.tile        = BODY_TILE(); // body is always header + 9
    player.sprite_body = sprites_register(sprite);

#if DEBUG
    sprite.tile = 4;
    player.debug1 = sprites_register(sprite);
    player.debug2 = sprites_register(sprite);
#endif

    vec2_set(player.direction, DIRECTION_NONE, DIRECTION_NONE);
    rect_set_pointer(&player.rect, 0, 0, SPRITE_WIDTH, SPRITE_HEIGHT);
    player_set_xy(WORLD_PLAYER_X, WORLD_PLAYER_Y);
    vec2_copy_pointer(&player.last_point, &player.point);

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

    // TODO: when stop moving, display "idle" (standing) frame

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
    vec2_copy_pointer(&player.last_point, &player.point);

    // not moving
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


bool player_pickup(int16_t type) {
    switch (type) {
        case TILE_UNKNOWN: // fall thru
        case TILE_WALL:
            return false;
    }

    if ((type >= GATES_INDEX) && (type <= (GATES_INDEX + keys_len))) {
        return false;
    }

    if ((type >= KEYS_INDEX) && (type <= (KEYS_INDEX + keys_len))) {
        uint8_t value = type - KEYS_INDEX;
        object_t* obj = &keys[value];
        obj->flags    = OBJECT_INACTIVE;
        room_set_tile(obj->x, obj->y, TILE_NONE); // make it a ground tile

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
        return true;
    }

    uint8_t roomId = ROOM_ID();

    if (type == FUEL_INDEX) {
        object_t* obj;
        for (uint8_t i = 0; i < fuel_len; i++) {
            obj = &fuel[i];
            if (obj->room == roomId) {
                obj->flags = OBJECT_INACTIVE;
                room_set_tile(obj->x, obj->y, TILE_NONE); // make it a ground tile

                player.energy += FUEL_VALUE;
                if (player.energy > MAX_ENERGY)
                    player.energy = MAX_ENERGY;

                hud_energy(player.energy);
                break;
            }
        }
        return true;
    }

    return false;
}

Edge player_collide(void) {
    Tile head_edge_tile, body_edge_tile;
    int16_t head_tile, body_tile;
    Edge edge = EdgeNone;

    Rect head_rect, body_rect;
    rect_set_pointer(&head_rect, player.point.x, player.point.y, player.size.w, player.size.h);
    rect_set_pointer(&body_rect, player.point.x, player.point.y, player.size.w, player.size.h);
    body_rect.y += SPRITE_HEIGHT;

    if(player.direction.x < 0) {
        if(tile_get_edge(&head_rect, EdgeLeft, &head_edge_tile) && tile_get_edge(&body_rect, EdgeLeft, &body_edge_tile)) {
            head_tile = tilemap_get(&head_edge_tile.point);
            body_tile = tilemap_get(&body_edge_tile.point);

#if DEBUG
            player.debug1->x = head_edge_tile.rect.x;
            player.debug1->y = head_edge_tile.rect.y;
            player.debug2->x = body_edge_tile.rect.x;
            player.debug2->y = body_edge_tile.rect.y;
#endif

            if((head_tile < 0) | (body_tile < 0)) {
                return EdgeLeft;
            } else if((head_tile != TILE_NONE) || (body_tile != TILE_NONE)) {
                bool head_blocked = head_tile != TILE_NONE && !player_pickup(head_tile);
                bool body_blocked = body_tile != TILE_NONE && !player_pickup(body_tile);
                if(head_blocked || body_blocked) {
                    player_set_xy(head_edge_tile.rect.x, player.point.y);
                }
            }
        } else {
            return EdgeLeft;
        }
    } else if(player.direction.x > 0) {
        if(tile_get_edge(&head_rect, EdgeRight, &head_edge_tile) && tile_get_edge(&body_rect, EdgeRight, &body_edge_tile)) {
            head_tile = tilemap_get(&head_edge_tile.point);
            body_tile = tilemap_get(&body_edge_tile.point);

#if DEBUG
            player.debug1->x = head_edge_tile.rect.x;
            player.debug1->y = head_edge_tile.rect.y;
            player.debug2->x = body_edge_tile.rect.x;
            player.debug2->y = body_edge_tile.rect.y;
#endif

            if((head_tile < 0) | (body_tile < 0)) {
                return EdgeRight;
            } else if((head_tile != TILE_NONE) || (body_tile != TILE_NONE)) {
                bool head_blocked = head_tile != TILE_NONE && !player_pickup(head_tile);
                bool body_blocked = body_tile != TILE_NONE && !player_pickup(body_tile);
                if(head_blocked || body_blocked) {
                    player_set_xy(head_edge_tile.rect.x, player.point.y);
                }
            }
        }
    }

    if(player.direction.y < 0) {
        if(tile_get_edge(&head_rect, EdgeTop, &head_edge_tile)) {
            head_tile = tilemap_get(&head_edge_tile.point);

#if DEBUG
            player.debug1->x = head_edge_tile.rect.x;
            player.debug1->y = head_edge_tile.rect.y;
            player.debug2->x = SCREEN_WIDTH + SPRITE_WIDTH;
#endif

            if(head_tile < 0) {
                return EdgeTop;
            } else if(head_tile != TILE_NONE) {
                if(!player_pickup(head_tile)) {
                    player_set_xy(player.point.x, head_edge_tile.rect.y);
                }
            }
        }
    } else if(player.direction.y > 0) {
        if(tile_get_edge(&body_rect, EdgeBottom, &body_edge_tile)) {
            body_tile = tilemap_get(&body_edge_tile.point);

#if DEBUG
            player.debug1->x = body_edge_tile.rect.x;
            player.debug1->y = body_edge_tile.rect.y;
            player.debug2->x = SCREEN_WIDTH + SPRITE_WIDTH;
#endif

            if(body_tile < 0) {
                return EdgeBottom;
            } else if(body_tile != TILE_NONE) {
                if(!player_pickup(body_tile)) {
                    player_set_xy(player.point.x, body_edge_tile.rect.y - SPRITE_HEIGHT);
                }
            }
        }
    }

    return EdgeNone;
}

void player_set_xy(uint16_t x, uint16_t y) {
    vec2_set_pointer(&player.point, x, y);
    sprite_set_xy(player.sprite_head, x, y);
    sprite_set_xy(player.sprite_body, x, y + SPRITE_HEIGHT);
}

inline void player_set_point(Point *point) {
    player_set_xy(point->x, point->y);
}

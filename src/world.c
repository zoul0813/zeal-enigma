#include <stdio.h>
#include <zgdk/utils/print.h>
#include <zgdk/utils/rand.h>
#include <zgdk/tilemap.h>
#include <zgdk/tilemap/scroll.h>
#include <zgdk/types.h>

#include "world.h"
#include "assets.h"
#include "main.h"
#include "objects.h"
#include "enemies.h"

/**
 * It is assumed the viewport is always the screen 0,1 on the tilemap
 */

#define SLIDE_SPEED 4

Vector2_u8 world_position;

TileType TILES[ROOM_HEIGHT * ROOM_WIDTH];
Tilemap ROOM;

void room_set_palette(void)
{
    // palette swap
    uint8_t palette_offset = ((world_position.x * world_position.y) % 9) + 4;
    uint16_t new_palette[2];
    new_palette[0] = palette[palette_offset];
    new_palette[1] = palette[(palette_offset + 1) % PALETTE_SIZE];
    if (new_palette[1] < 4)
        new_palette[1] += 4;

    gfx_palette_load(&vctx, &new_palette, sizeof(new_palette), 1);
}

void room_render_keys(void)
{
    uint8_t roomId = ROOM_ID();
    for (uint8_t i = 0; i < keys_len; i++) {
        object_t* key = &keys[i];
        if (key->room != roomId)
            continue;
        if (key->flags != OBJECT_ACTIVE)
            continue;
        gfx_tilemap_place(&vctx, KEYS_INDEX + i, LAYER0, key->x + ROOM_OFFSET_X, key->y + ROOM_OFFSET_Y);
        tilemap_set_xy(key->x, key->y, KEYS_INDEX + i);
    }
}

void room_render_fuel(void)
{
    uint8_t roomId = ROOM_ID();
    for (uint8_t i = 0; i < fuel_len; i++) {
        object_t* can = &fuel[i];
        if (can->room != roomId)
            continue;
        if (can->flags != OBJECT_ACTIVE)
            continue;
        gfx_tilemap_place(&vctx, FUEL_INDEX, LAYER0, can->x + ROOM_OFFSET_X, can->y + ROOM_OFFSET_Y);
        tilemap_set_xy(can->x, can->y, FUEL_INDEX);
    }
}

void room_render_gate(void)
{
    uint8_t roomId = ROOM_ID();
    for (uint8_t i = 0; i < gates_len; i++) {
        object_t* gate = &gates[i];
        if (gate->room != roomId)
            continue;
        if (!(gate->flags & OBJECT_ACTIVE))
            continue;

        if ((gate->flags & OBJECT_VERTICAL)) {
            for (uint8_t j = 0; j < GATE_LENGTH; j++) {
                gfx_tilemap_place(&vctx, gate->id + GATES_INDEX, LAYER0, gate->x + ROOM_OFFSET_X,
                                  gate->y + j + ROOM_OFFSET_Y);
                tilemap_set_xy(gate->x, gate->y + j, GATES_INDEX + gate->id);
            }
        } else {
            for (uint8_t j = 0; j < GATE_LENGTH; j++) {
                gfx_tilemap_place(&vctx, gate->id + GATES_INDEX, LAYER0, gate->x + j + ROOM_OFFSET_X,
                                  gate->y + ROOM_OFFSET_Y);
                tilemap_set_xy(gate->x + j, gate->y, GATES_INDEX + gate->id);
            }
        }
    }
}

void room_set_tile(uint8_t x, uint8_t y, uint8_t tile)
{
    // out of bounds, ignore
    if (x >= ROOM_WIDTH)
        return;
    if (y >= ROOM_HEIGHT)
        return;

    // place tile on screen
    tilemap_place_xy(&vctx, LAYER0, tile, ROOM_OFFSET_X + x, ROOM_OFFSET_Y + y);
    tilemap_set_xy(x, y, TILE_NONE);
}

/** WORLD FUNCTIONS */

error world_init(void)
{
    rect_set_pointer(&ROOM.rect, ROOM_OFFSET_X, ROOM_OFFSET_Y, ROOM_WIDTH, ROOM_HEIGHT);
    tilemap_register(&ROOM, TILES);

    // load the screen the player starts on
    world_position.x = WORLD_START_X;
    world_position.y = WORLD_START_Y;

    Point point;
    screen_load(world_position, EdgeNone, point, NULL);

    tilemap_scroll_x(0, 0);
    return ERR_SUCCESS;
}


/** SCREEN FUNCTIONS */

void screen_load(const Vector2_u8 world, const Edge edge, const Point point, transition_callback_t const callback)
{
    // update to the latest
    world_position.x = world.x;
    world_position.y = world.y;

    room_set_palette();

    // load the current screen into 0,0 and jump to it
    gfx_wait_vblank(&vctx);
    load_tilemap(&vctx, world.x, world.y, 0, 0);
    gfx_wait_end_vblank(&vctx);

    room_render_keys();
    room_render_fuel();
    room_render_gate();
    enemies_spawn(ROOM_ID());

    uint8_t roomId = ROOM_ID();
    sprintf(buffer, "%02d", roomId);
    nprint_string_layer(&vctx, buffer, 2, LAYER0, 0, HEIGHT - 1);

    gfx_wait_vblank(&vctx);
    if(callback != NULL) {
        switch (edge) {
            case EdgeLeft: callback(ROOM_RIGHT_PX, point.y); break;
            case EdgeRight: callback(ROOM_LEFT_PX + SPRITE_WIDTH, point.y); break;
            case EdgeTop: callback(point.x, ROOM_BOTTOM_PX - SPRITE_HEIGHT); break;
            case EdgeBottom: callback(point.x, ROOM_TOP_PX + SPRITE_HEIGHT); break;
        }
    }
    gfx_wait_end_vblank(&vctx);
}

/**
 * Transition to the next screen
 *
 * This will momentarily "pause" the game and transition
 * the mext screen into view, if applicable.
 *
 * @param edge The edge of the current screen the player is exiting from
 * @returns 1 if transition occurred, 0 is no transition occurred
 */
uint8_t screen_transition(const Edge edge, const Point point, const transition_callback_t callback)
{
    uint8_t transition = 0;

    Vector2_u8 new_world = {
        .x = world_position.x,
        .y = world_position.y,
    };

    switch (edge) {
        case EdgeLeft:
            if (new_world.x > 0) {
                new_world.x--;
                transition = 1;
            } else {
                // don't let the player go off screen
                callback(SPRITE_WIDTH + ROOM_OFFSET_X, point.y);
            }
            break;
        case EdgeRight:
            if (new_world.x < (WORLD_WIDTH - 1)) {
                new_world.x++;
                transition = 1;
            } else {
                // don't let the player go off screen
                callback((SPRITE_WIDTH * ROOM_WIDTH) + ROOM_OFFSET_X, point.y);
            }
            break;
        case EdgeTop:
            if (new_world.y > 0) {
                new_world.y--;
                transition = 1;
            } else {
                // don't let the player go off screen
                callback(point.x, SPRITE_HEIGHT + ROOM_OFFSET_Y);
            }
            break;
        case EdgeBottom:
            if (new_world.y < (WORLD_HEIGHT - 1)) {
                new_world.y++;
                transition = 1;
            } else {
                // don't let the player go off screen
                callback(point.x, (SPRITE_HEIGHT * ROOM_HEIGHT) + ROOM_OFFSET_Y);
            }
            break;
    }

    if (transition) {
        screen_load(new_world, edge, point, callback);
        return 1;
    }

    return 0;
}

#include <stdio.h>
#include <zgdk/utils/print.h>
#include <zgdk/tilemap.h>
#include <zgdk/tilemap/scroll.h>
#include <world/world.h>
#include <world/transitions.h>

#include "game_world.h"
#include "assets.h"
#include "objects.h"
#include "enemies.h"
#include "player.h"

static uint8_t room_tiles[ROOM_WIDTH * ROOM_HEIGHT];
static void room_entered(gfx_context* ctx, uint8_t x, uint8_t y, Edge edge);

static const world_config_t world_config = {
    .size = { WORLD_WIDTH, WORLD_HEIGHT },
    .screen = { SCREEN_WIDTH, SCREEN_HEIGHT },
    .room = { .x = ROOM_OFFSET_X, .y = ROOM_OFFSET_Y, .w = ROOM_WIDTH, .h = ROOM_HEIGHT },
    .layer = LAYER0,
    .tiles = room_tiles,
    .tiles_size = sizeof(room_tiles),
    .load_room = load_tilemap,
    .on_enter = room_entered,
    .transition = NULL,
};

static void room_set_palette(void)
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

static void room_render_keys(void)
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

static void room_render_fuel(void)
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

static void room_render_gate(void)
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

/* Game-owned room entry: world.lib has committed the destination identity
 * and registered its logical tiles before invoking this callback once. */
static void room_entered(gfx_context* ctx, uint8_t x, uint8_t y, Edge edge) {
    (void)x; (void)y;
    room_set_palette();
    room_render_keys();
    room_render_fuel();
    room_render_gate();
    enemies_spawn(ROOM_ID());

    sprintf(buffer, "%02d", ROOM_ID());
    nprint_string_layer(ctx, buffer, 2, LAYER0, 0, HEIGHT - 1);

    gfx_wait_vblank(ctx);
    switch (edge) {
        case EdgeLeft: player_set_xy(ROOM_RIGHT_PX, player.point.y); break;
        case EdgeRight: player_set_xy(ROOM_LEFT_PX + SPRITE_WIDTH, player.point.y); break;
        case EdgeTop: player_set_xy(player.point.x, ROOM_BOTTOM_PX - SPRITE_HEIGHT); break;
        case EdgeBottom: player_set_xy(player.point.x, ROOM_TOP_PX + SPRITE_HEIGHT); break;
        default: break;
    }
    gfx_wait_end_vblank(ctx);
}

zos_err_t game_world_init(void) {
    Vector2_u8 start = { WORLD_START_X, WORLD_START_Y };
    return world_init(&vctx, &world_config, start);
}

zos_err_t game_world_transition(Edge edge) {
    int16_t moved = world_move(edge);
    if (moved < 0) return (zos_err_t)-moved;
    if (!moved) {
        // Clamp to the actual room viewport, accounting for both player tiles.
        switch (edge) {
            case EdgeLeft: player_set_xy(ROOM_LEFT_PX + SPRITE_WIDTH, player.point.y); break;
            case EdgeRight: player_set_xy(ROOM_RIGHT_PX, player.point.y); break;
            case EdgeTop: player_set_xy(player.point.x, ROOM_TOP_PX + SPRITE_HEIGHT); break;
            case EdgeBottom: player_set_xy(player.point.x, ROOM_BOTTOM_PX - SPRITE_HEIGHT); break;
            default: break;
        }
    }
    return ERR_SUCCESS;
}

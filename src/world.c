#include "world.h"

#include "assets.h"
#include "main.h"

/**
 * It is assumed the viewport is always the screen 0,1 on the tilemap
 */

#define SLIDE_SPEED  4

Vector2_u8 world_position;
TileType SCREEN[ROOM_HEIGHT][ROOM_WIDTH];

/** WORLD FUNCTIONS */

error world_init(gfx_context* ctx)
{
    // load the screen the player starts on
    world_position.x = WORLD_START_X;
    world_position.y = WORLD_START_Y;
    zos_err_t err    = load_tilemap(ctx, world_position.x, world_position.y, 0, 0);
    if (err != ERR_SUCCESS)
        return err;
    tilemap_scroll_x(0, 0);
    return ERR_SUCCESS;
}

void world_set_tile(uint8_t x, uint8_t y, uint8_t tile)
{
    // out of bounds, ignore
    if (x >= WIDTH)
        return;
    if (y >= HEIGHT)
        return;

    // place tile on screen
    gfx_tilemap_place(&vctx, tile, 0, x + WIDTH, y);
    SCREEN[y][x] = TILE_OPEN;

    // place tile into world
    data[(y * WIDTH) + x] = tile;
}

/** SCREEN FUNCTIONS */

void screen_load(const Vector2_u8 world, const Edge edge, const Point point, transition_callback_t const callback)
{
    // palette swap
    uint8_t palette_offset = ((world.x * world.y) % 9) + 4;
    uint16_t new_palette[2] = {
        palette[palette_offset],
        palette[(palette_offset + 3) % 16],
    };

    // load the current screen into 0,0 and jump to it
    gfx_wait_vblank(&vctx);
    gfx_palette_load(&vctx, &new_palette, sizeof(new_palette), 1);
    load_tilemap(&vctx, world.x, world.y, 0, 0);
    gfx_wait_end_vblank(&vctx);

    gfx_wait_vblank(&vctx);
    switch (edge) {
        case EdgeLeft: callback(ROOM_RIGHT_PX, point.y); break;
        case EdgeRight: callback(ROOM_LEFT_PX + SPRITE_WIDTH, point.y); break;
        case EdgeTop: callback(point.x, ROOM_BOTTOM_PX); break;
        case EdgeBottom: callback(point.x, ROOM_TOP_PX + SPRITE_HEIGHT); break;
    }
    gfx_wait_end_vblank(&vctx);

    // update to the latest
    world_position.x = world.x;
    world_position.y = world.y;
}

TileType screen_get_tile(const uint8_t x, const uint8_t y)
{
    // out of bounds, assume TILE_OPEN
    if (x >= WIDTH)
        return TILE_OPEN;
    if (y >= HEIGHT)
        return TILE_OPEN;
    return SCREEN[y][x];
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
                callback(SPRITE_WIDTH, point.y);
            }
            break;
        case EdgeRight:
            if (new_world.x < (WORLD_WIDTH - 1)) {
                new_world.x++;
                transition = 1;
            } else {
                // don't let the player go off screen
                callback(SCREEN_WIDTH, point.y);
            }
            break;
        case EdgeTop:
            if (new_world.y > 0) {
                new_world.y--;
                transition = 1;
            } else {
                // don't let the player go off screen
                callback(point.x, SPRITE_HEIGHT);
            }
            break;
        case EdgeBottom:
            if (new_world.y < (WORLD_HEIGHT - 1)) {
                new_world.y++;
                transition = 1;
            } else {
                // don't let the player go off screen
                callback(point.x, SCREEN_HEIGHT);
            }
            break;
    }

    if (transition) {
        screen_load(new_world, edge, point, callback);
        return 1;
    }

    return 0;
}

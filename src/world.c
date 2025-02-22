#include "world.h"
#include "assets.h"
#include "main.h"
#include "player.h"

/**
 * It is assumed the viewport is always the screen 0,1 on the tilemap
 */

#define SLIDE_SPEED 4

Vector2_u8 world_position;
TileType SCREEN[HEIGHT][WIDTH];

/** WORLD FUNCTIONS */

error world_init(gfx_context *ctx) {
    // load the screen the player starts on
    world_position.x = 0;
    world_position.y = 0;
    zos_err_t err = load_tilemap(ctx, world_position.x, world_position.y, 0, 0);
    if(err != ERR_SUCCESS) return err;
    tilemap_scroll_x(0, 0);
    return ERR_SUCCESS;
}

void world_set_tile(uint8_t x, uint8_t y, uint8_t tile) {
    // place tile on screen
    gfx_tilemap_place(&vctx, tile, 0, x + WIDTH, y);
    SCREEN[y][x] = TILE_OPEN;

    // place tile into world
    data[(y * WIDTH) + x] = tile;
}



/** SCREEN FUNCTIONS */

void screen_slide(Point tilemap, const Edge world_edge, const Edge player_edge) {
    uint8_t frames = 0;

    Direction world_direction = {
        .x = 0,
        .y = 0,
    };

    Direction player_direction = {
        .x = 0,
        .y = 0,
    };

    Point player_pos = {
        .x = player.point.x,
        .y = player.point.y,
    };

    // determine direction/timing of screen
    switch(world_edge) {
        case EdgeLeft:
            world_direction.x = DIRECTION_LEFT * SLIDE_SPEED;
            frames = SCREEN_WIDTH / SLIDE_SPEED;
            break;
        case EdgeRight:
            world_direction.x = DIRECTION_RIGHT * SLIDE_SPEED;
            frames = SCREEN_WIDTH / SLIDE_SPEED;
            break;
        case EdgeTop:
            world_direction.y = DIRECTION_UP * SLIDE_SPEED;
            frames = SCREEN_HEIGHT / SLIDE_SPEED; // this will cause v-slides to be faster than h-slides :think:
            break;
        case EdgeBottom:
            world_direction.y = DIRECTION_DOWN * SLIDE_SPEED;
            frames = SCREEN_HEIGHT / SLIDE_SPEED; // this will cause v-slides to be faster than h-slides :think:
            break;
    }

    // determine direction of player
    switch(player_edge) {
        case EdgeLeft:
            player_direction.x = DIRECTION_LEFT * SLIDE_SPEED;
            break;
        case EdgeRight:
            player_direction.x = DIRECTION_RIGHT * SLIDE_SPEED;
            break;
        case EdgeTop:
            player_direction.y = DIRECTION_UP * SLIDE_SPEED;
            break;
        case EdgeBottom:
            player_direction.y = DIRECTION_DOWN * SLIDE_SPEED;
            break;
    }

    // slide the player the few extra pixels
    for(uint8_t x = SPRITE_WIDTH/SLIDE_SPEED; x > 0; x--) {
        player_pos.x += player_direction.x;
        player_pos.y += player_direction.y;
        player_set_xy(player_pos.x, player_pos.y);

        gfx_wait_vblank(&vctx);
        sprites_render(&vctx);
        gfx_wait_end_vblank(&vctx);
    }

    // slide the player/room into view
    for(uint8_t x = frames; x > 0; x--) {
        tilemap.x += world_direction.x;
        tilemap.y += world_direction.y;

        player_pos.x += -player_direction.x;
        player_pos.y += -player_direction.y;
        player_set_xy(player_pos.x, player_pos.y);

        gfx_wait_vblank(&vctx);

        tilemap_scroll(0, tilemap.x, tilemap.y);
        sprites_render(&vctx);

        gfx_wait_end_vblank(&vctx);
    }

    // make sure the player is positioned properly
    switch (player_edge) {
        case EdgeLeft:
            player_set_xy(SCREEN_WIDTH, player.point.y);
            break;
        case EdgeRight:
            player_set_xy(SPRITE_WIDTH, player.point.y);
            break;
        case EdgeTop:
            player_set_xy(player.point.x, SCREEN_HEIGHT);
            break;
        case EdgeBottom:
            player_set_xy(player.point.x, SPRITE_HEIGHT);
            break;
    }
}


void screen_load(Vector2_u8 world, Edge edge) {
    Vector2_u8 old_world = {
        .x = world_position.x,
        .y = world_position.y,
    };

    Edge world_edge = edge;
    Edge player_edge = edge;

    Point tilemap = {
        .x = SCREEN_WIDTH,
        .y = 0,
    };

    switch(edge) {
        case EdgeLeft:
            load_tilemap(&vctx, world.x, world.y, 0, 0);
            break;
        case EdgeRight:
            load_tilemap(&vctx, world.x, world.y, 2, 0);
            break;
        case EdgeTop:
            // load the old map beneath
            load_tilemap(&vctx, old_world.x, old_world.y, 1, 1);
            // jump to old map
            gfx_wait_vblank(&vctx);
            tilemap_scroll(0, SCREEN_WIDTH, SCREEN_HEIGHT);
            gfx_wait_end_vblank(&vctx);

            // load the new map above
            load_tilemap(&vctx, world.x, world.y, 1, 0);

            tilemap.y = SCREEN_HEIGHT;
            break;
        case EdgeBottom:
            load_tilemap(&vctx, world.x, world.y, 1, 1);
            break;
    }

    screen_slide(tilemap, world_edge, player_edge);

    // load the current screen into 1,0 and jump to it
    load_tilemap(&vctx, world.x, world.y, 1, 0);
    gfx_wait_vblank(&vctx);
    tilemap_scroll(0, SCREEN_WIDTH, 0);
    gfx_wait_end_vblank(&vctx);

    // update to the latest
    world_position.x = world.x;
    world_position.y = world.y;
}

TileType screen_get_tile(uint8_t x, uint8_t y) {
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
uint8_t screen_transition(const Edge edge) {
    uint8_t transition = 0;

    Vector2_u8 new_world = {
        .x = world_position.x,
        .y = world_position.y,
    };

    switch (edge) {
        case EdgeLeft:
            if (new_world.x > 0) {
                new_world.x--;
                transition=1;
            } else {
                // don't let the player go off screen
                player_set_xy(SPRITE_WIDTH, player.point.y);
            }
            break;
        case EdgeRight:
            if (new_world.x < (WORLD_WIDTH - 1)) {
                new_world.x++;
                transition=1;
            } else {
                // don't let the player go off screen
                player_set_xy(SCREEN_WIDTH, player.point.y);
            }
            break;
        case EdgeTop:
            if (new_world.y > 0) {
                new_world.y--;
                transition=1;
            } else {
                // don't let the player go off screen
                player_set_xy(player.point.x, SPRITE_HEIGHT);
            }
            break;
        case EdgeBottom:
            if (new_world.y < (WORLD_HEIGHT - 1)) {
                new_world.y++;
                transition=1;
            } else {
                // don't let the player go off screen
                player_set_xy(player.point.x, SCREEN_HEIGHT);
            }
            break;
    }

    if(transition) {
        screen_load(new_world, edge);
        return 1;
    }

    return 0;
}


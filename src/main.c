#include <stdio.h>
#include <stdint.h>
#include <zos_errors.h>
#include <zos_vfs.h>
#include <zos_sys.h>
#include <zos_video.h>
#include <zgdk.h>

#include "assets.h"
#include "world.h"
#include "player.h"
#include "main.h"

gfx_context vctx;

int main(void) {
    init();
reset:
    reset();
    load_level(0);
    while(true) {
        uint8_t action = input();
        if(action == ACTION_QUIT) goto quit_game;
        update();
        draw();
    }
quit_game:
    deinit();
    return 0;
}

void handle_error(zos_err_t err, const char* message, uint8_t fatal) {
    if(err != ERR_SUCCESS) {
        if(fatal) deinit();
        printf("\nError[%d] (%02x) failed to %s", err, err, message);
        if(fatal) exit(err);
    }
}

void init(void) {
    zos_err_t err;
    err = keyboard_init();
    handle_error(err, "init keyboard", 1);
    err = keyboard_flush();
    handle_error(err, "flush keyboard", 1);

    err = load_asset_file("enigma.zar");

    // disable the screen to prevent artifacts from showing
    gfx_enable_screen(0);

    err = gfx_initialize(ZVB_CTRL_VID_MODE_GFX_640_4BIT, &vctx);
    handle_error(err, "init graphics", 1);

    err = load_palette(&vctx);
    handle_error(err, "load palette", 1);

    gfx_tileset_options options = {
        .compression = TILESET_COMP_4BIT,
    };
    err = load_tileset(&vctx, &options);
    handle_error(err, "load tileset", 1);

    err = player_init();
    handle_error(err, "init player", 1);

    clear_tilemap(0, 0, 80, 60, 0);  // clear all layer0 memory
    clear_tilemap(0, 0, 80, 60, 1);  // clear all layer1 memory

    err = world_init(&vctx);
    handle_error(err, "initialize world", 1);

    gfx_enable_screen(1);
}

void deinit(void) {
    // reset screen
    ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);

    zos_err_t err = zar_file_close(&zar_file);
    handle_error(err, "close zar", 0);

    err = player_deinit();
    handle_error(err, "deinit player", 0);
}

void reset(void) {
    // reset your game state
}

uint8_t input(void) {
    uint16_t input1 = keyboard_read();

    player.direction.x = DIRECTION_NONE;
    player.direction.y = DIRECTION_NONE;
    if(LEFT1) player.direction.x = DIRECTION_LEFT;
    if(RIGHT1) player.direction.x = DIRECTION_RIGHT;
    if(UP1) player.direction.y = DIRECTION_UP;
    if(DOWN1) player.direction.y = DIRECTION_DOWN;

    if(SELECT1) return ACTION_QUIT;

    return 0;
}


void clear_tilemap(uint8_t layer, uint8_t x, uint8_t y, uint16_t width, uint16_t height) {
    uint8_t line[80];
    for (uint16_t row = 0; row < height; row++) {
        for (uint16_t col = 0; col < width; col++) {
            line[col] = 0xFF;
        }
        gfx_error err = gfx_tilemap_load(&vctx, line, width, layer, x, y + row);
        handle_error(err, "clear tilemap", 1);
    }
}

error load_level(uint8_t which) {
    (void *)which; // unreferenced
    return 0;
}

void update(void) {
    player_move();
}

void draw_paused(uint8_t paused) {
    (void*) paused; // unreferenced
}

void draw_gameover(uint8_t gameover) {
    (void*) gameover; // unreferenced
}

void draw(void) {
    gfx_wait_vblank(&vctx);
    sprites_render(&vctx);
    gfx_wait_end_vblank(&vctx);
}
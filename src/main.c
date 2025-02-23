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
uint8_t buffer[SCREEN_WIDTH];

void handle_error(zos_err_t err, const char* message, uint8_t fatal) {
    if(err != ERR_SUCCESS) {
        if(fatal) deinit();
        printf("\nError[%d] (%02x) failed to %s", err, err, message);
        if(fatal) exit(err);
    }
}

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

void init(void) {
    zos_err_t err;
    err = keyboard_init();
    handle_error(err, "init keyboard", 1);
    err = keyboard_flush();
    handle_error(err, "flush keyboard", 1);

    err = load_asset_file("enigma.zar");

    sprites_register_arena(SPRITE_ARENA, MAX_SPRITES);

    ascii_map(' ', 1, 138); // empty tile after Z
    ascii_map('A', 16, 112);
    ascii_map('a', 16, 112);
    ascii_map('Q', 10, 128);
    ascii_map('q', 10, 128);
    ascii_map('0', 11, 144); // includes :
    ascii_map('-', 1, 155);
    ascii_map('*', 1, 156); // player icon

    // disable the screen to prevent artifacts from showing
    gfx_enable_screen(0);

    err = gfx_initialize(ZVB_CTRL_VID_MODE_GFX_640_4BIT, &vctx);
    handle_error(err, "init graphics", 1);

    clear_tilemap(LAYER0, EMPTY_TILE, 0, 0, WIDTH, HEIGHT);  // clear all layer0 memory
    clear_tilemap(LAYER1, 0, 0, 0, WIDTH, HEIGHT);

    err = load_palette(&vctx, "tileset.ztp", 0);
    handle_error(err, "load palette", 1);

    uint16_t tile_offset = 0;
    gfx_tileset_options options = {
        .compression = TILESET_COMP_4BIT,
        .pal_offset = 0,
        .from_byte = 0,
    };
    err = load_tileset(&vctx, "tileset.zts", &options);
    handle_error(err, "load tileset.zts", 1);

    tile_offset += 16;
    options.from_byte = tile_offset * 128;
    err = load_tileset(&vctx, "player.zts", &options);
    handle_error(err, "load player.zts", 1);

    tile_offset += 32;
    options.from_byte = tile_offset * 128;
    err = load_tileset(&vctx, "enemies.zts", &options);
    handle_error(err, "load enemies.zts", 1);

    tile_offset += 32;
    options.from_byte = tile_offset * 128;
    err = load_tileset(&vctx, "gates.zts", &options);
    handle_error(err, "load gates.zts", 1);

    tile_offset += 32;
    options.from_byte = tile_offset * 128;
    err = load_tileset(&vctx, "letters.zts", &options);
    handle_error(err, "load letters.zts", 1);

    tile_offset += 32;
    options.from_byte = tile_offset * 128;
    err = load_tileset(&vctx, "numbers.zts", &options);
    handle_error(err, "load numbers.zts", 1);

    err = player_init();
    handle_error(err, "init player", 1);

    err = world_init(&vctx);
    handle_error(err, "initialize world", 1);

    sprintf(buffer, "1985 Enigma by CBM UK Ltd");
    nprint_string_layer(&vctx, buffer, 25, LAYER0, 2, 0);

    sprintf(buffer, "2025 Port for Zeal 8-bit Computer");
    nprint_string_layer(&vctx, buffer, 33, LAYER0, 2, 1);

    sprintf(buffer, "Code: zoul0813 Graphics: Arcadia");
    nprint_string_layer(&vctx, buffer, 32, LAYER0, 2, 2);

    sprintf(buffer, "Arrows: Move  Space: Shoot");
    nprint_string_layer(&vctx, buffer, 26, LAYER0, 2, 3);

    // TODO: make this a HUD update function
    sprintf(buffer, "LIVES: ***   KEYS:");
    nprint_string_layer(&vctx, buffer, 18, LAYER0, 2, HEIGHT-4);

    sprintf(buffer, "SCORE: %05d      HI-SCORE: %05d", 0, 0);
    nprint_string_layer(&vctx, buffer, 33, LAYER0, 2, HEIGHT-3);
    sprintf(buffer, "ENERGY:");
    nprint_string_layer(&vctx, buffer, 7, LAYER0, 2, HEIGHT-2);

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

void clear_tilemap(uint8_t layer, uint8_t tile, uint8_t x, uint8_t y, uint16_t width, uint16_t height) {
    uint8_t line[80];
    for (uint16_t row = 0; row < height; row++) {
        for (uint16_t col = 0; col < width; col++) {
            line[col] = tile;
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
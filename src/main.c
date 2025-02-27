#include <stdio.h>
#include <stdint.h>
#include <zos_errors.h>
#include <zos_vfs.h>
#include <zos_sys.h>
#include <zos_video.h>
#include <zgdk.h>

#include "main.h"
#include "hud.h"
#include "assets.h"
#include "objects.h"
#include "world.h"
#include "player.h"
#include "bullet.h"
#include "enemies.h"

gfx_context vctx;
uint8_t buffer[WIDTH];

int main(void)
{
    init();
reset:
    reset();

    while (true) {
        uint8_t action = input();
        if (action == ACTION_QUIT)
            goto quit_game;
        update();
        draw();
    }
quit_game:
    deinit();
    return 0;
}

void handle_error(zos_err_t err, const char* message, uint8_t fatal)
{
    if (err != ERR_SUCCESS) {
        if (fatal)
            deinit();
        printf("\nError[%d] (%02x) failed to %s\n", err, err, message);
        if (fatal)
            exit(err);
    }
}

void init(void)
{
    zos_err_t err;
    err = input_init(1);
    handle_error(err, "init keyboard", 1);
    input_flush();

    err = load_asset_file("enigma.zar");
    handle_error(err, "load zar", 1);

    rand8_seed(rand8_quick());

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

    clear_tilemap(LAYER0, 0, 0, 0, WIDTH, HEIGHT); // clear all layer0 memory
    clear_tilemap(LAYER1, 0, 0, 0, WIDTH, HEIGHT);

    err = load_palette(&vctx, "tileset.ztp", 0);
    handle_error(err, "load palette", 1);

    // err = gfx_tileset_add_color_tile(&vctx, EMPTY_TILE, 0);
    // handle_error(err, "create empty tile", 1);

    uint16_t tile_offset        = 0;
    gfx_tileset_options options = {
        .compression = TILESET_COMP_4BIT,
        .pal_offset  = 0,
        .from_byte   = 0,
    };
    err = load_tileset(&vctx, "tileset.zts", &options);
    handle_error(err, "load tileset.zts", 1);

    tile_offset       += 16;
    options.from_byte  = tile_offset * 128;
    err                = load_tileset(&vctx, "player.zts", &options);
    handle_error(err, "load player.zts", 1);

    tile_offset       += 32;
    options.from_byte  = tile_offset * 128;
    err                = load_tileset(&vctx, "enemies.zts", &options);
    handle_error(err, "load enemies.zts", 1);

    tile_offset       += 32;
    options.from_byte  = tile_offset * 128;
    err                = load_tileset(&vctx, "gates.zts", &options);
    handle_error(err, "load gates.zts", 1);

    tile_offset       += 32;
    options.from_byte  = tile_offset * 128;
    err                = load_tileset(&vctx, "letters.zts", &options);
    handle_error(err, "load letters.zts", 1);

    tile_offset       += 32;
    options.from_byte  = tile_offset * 128;
    err                = load_tileset(&vctx, "numbers.zts", &options);
    handle_error(err, "load numbers.zts", 1);

    err = player_init();
    handle_error(err, "init player", 1);

    err = bullet_init();
    handle_error(err, "init bullets", 1);

    err = enemies_init();
    handle_error(err, "init enemies", 1);

    // TODO: set red background for Banner/HUD
    clear_tilemap(LAYER1, RED_GREY, 0, 0, WIDTH, 4);
    clear_tilemap(LAYER1, RED_YELLOW, 0, HEIGHT - 5, WIDTH, 5);
    // clear_tilemap(LAYER1, 0, ROOM_OFFSET_X, ROOM_OFFSET_Y, ROOM_WIDTH, ROOM_HEIGHT);

    err = world_init(&vctx);
    handle_error(err, "initialize world", 1);

#if DEBUG
    sprintf(buffer, "%03d %03d %02d", world_position.x, world_position.y, ROOM_ID());
    nprint_string_layer(&vctx, buffer, 10, LAYER0, 0, 0);
#endif

#if !defined(DEBUG) || DEBUG == 0
    hud_init();
    hud_score(player.score);
    hud_lives(player.lives);
    hud_keys(player.keys);
    hud_energy(player.energy);
#endif

    gfx_enable_screen(1);
}

void deinit(void)
{
    // reset the tilemap scroll
    tilemap_scroll(0, 0, 0);
    tilemap_scroll(1, 0, 0);

    // reset screen
    ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);

    zos_err_t err = zar_file_close(&zar_file);
    handle_error(err, "close zar", 0);

    err = player_deinit();
    handle_error(err, "deinit player", 0);


    for(uint8_t y = 0; y < ROOM_HEIGHT; y++) {
        for(uint8_t x = 0; x < ROOM_WIDTH; x++) {
            printf("%02x", SCREEN[y][x]);
        }
        printf("\n");
    }
}

void reset(void)
{
    // reset your game state
}

static uint16_t input1_prev = 0;
uint8_t input(void)
{
    uint16_t input1 = input_get();

    if (input1 == input1_prev)
        return 0;
    input1_prev = input1;

    player.direction.x = DIRECTION_NONE;
    player.direction.y = DIRECTION_NONE;
    if (LEFT1)
        player.direction.x = DIRECTION_LEFT;
    if (RIGHT1)
        player.direction.x = DIRECTION_RIGHT;
    if (UP1)
        player.direction.y = DIRECTION_UP;
    if (DOWN1)
        player.direction.y = DIRECTION_DOWN;

    if (BUTTON1_B) {
        if ((player.energy > 0) && bullet_shoot(&player.point, &player.direction)) {
            player.energy--;
            hud_energy(player.energy);
        }
    }

    if (SELECT1)
        return ACTION_QUIT;

    return 0;
}

void clear_tilemap(uint8_t layer, uint8_t tile, uint8_t x, uint8_t y, uint16_t width, uint16_t height)
{
    uint8_t line[80];
    for (uint16_t row = 0; row < height; row++) {
        for (uint16_t col = 0; col < width; col++) {
            line[col] = tile;
        }
        gfx_error err = gfx_tilemap_load(&vctx, line, width, layer, x, y + row);
        handle_error(err, "clear tilemap", 1);
    }
}

void update(void)
{
    Edge screen_edge = player_move();
    bullet_move();
    enemies_move();
    if (screen_edge != EdgeNone) {
        screen_transition(screen_edge, player.point, player_set_xy);

#if DEBUG
        sprintf(buffer, "%03d %03d %02d", world_position.x, world_position.y, ROOM_ID());
        nprint_string_layer(&vctx, buffer, 10, LAYER0, 0, 0);
#endif
    }
}

void draw_paused(uint8_t paused)
{
    (void*) paused; // unreferenced
}

void draw_gameover(uint8_t gameover)
{
    (void*) gameover; // unreferenced
}

void draw(void)
{
    gfx_wait_vblank(&vctx);
    sprites_render(&vctx);
    gfx_wait_end_vblank(&vctx);
}

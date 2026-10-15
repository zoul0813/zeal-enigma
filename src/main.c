#include <stdio.h>
#include <stdint.h>
#include <zos_errors.h>
#include <zos_vfs.h>
#include <zos_sys.h>
#include <zos_video.h>
#include <zgdk.h>
#include <zgdk/tilemap/scroll.h>

#include "main.h"
#include "hud.h"
#include "assets.h"
#include <enigma_assets.h>
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
        TSTATE_LOG(1);
        uint8_t action = input();
        if (action == ACTION_QUIT)
            goto quit_game;
        update();
        draw();
        TSTATE_LOG(1);
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

    tilemap_fill(&vctx, LAYER0, 0, 0, 0, WIDTH, HEIGHT); // clear all layer0 memory
    tilemap_fill(&vctx, LAYER1, 0, 0, 0, WIDTH, HEIGHT);

    err = load_palette(&vctx, ZAR_TILESET_ZTP, 0);
    handle_error(err, "load palette", 1);

    // err = gfx_tileset_add_color_tile(&vctx, EMPTY_TILE, 0);
    // handle_error(err, "create empty tile", 1);

    uint16_t tile_offset        = 0;
    gfx_tileset_options options = {
        // .compression = TILESET_COMP_4BIT,
        .pal_offset  = 0,
        .from_byte   = 0,
    };
    err = load_tileset(&vctx, ZAR_TILESET_ZTS, &options);
    handle_error(err, "load tileset.zts", 1);

    tile_offset       += 16;
    options.from_byte  = tile_offset * 128;
    err                = load_tileset(&vctx, ZAR_PLAYER_ZTS, &options);
    handle_error(err, "load player.zts", 1);

    tile_offset       += 32;
    options.from_byte  = tile_offset * 128;
    err                = load_tileset(&vctx, ZAR_ENEMIES_ZTS, &options);
    handle_error(err, "load enemies.zts", 1);

    tile_offset       += 32;
    options.from_byte  = tile_offset * 128;
    err                = load_tileset(&vctx, ZAR_GATES_ZTS, &options);
    handle_error(err, "load gates.zts", 1);

    tile_offset       += 32;
    options.from_byte  = tile_offset * 128;
    err                = load_tileset(&vctx, ZAR_LETTERS_ZTS, &options);
    handle_error(err, "load letters.zts", 1);

    tile_offset       += 32;
    options.from_byte  = tile_offset * 128;
    err                = load_tileset(&vctx, ZAR_NUMBERS_ZTS, &options);
    handle_error(err, "load numbers.zts", 1);

    err = player_init();
    handle_error(err, "init player", 1);

    err = bullet_init();
    handle_error(err, "init bullets", 1);

    err = enemies_init();
    handle_error(err, "init enemies", 1);

    tilemap_fill(&vctx, LAYER1, RED_GREY, 0, 0, WIDTH, 4);
    tilemap_fill(&vctx, LAYER1, RED_YELLOW, 0, HEIGHT - 5, WIDTH, 5);

    err = world_init();
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
    tilemap_scroll(LAYER0, 0, 0);
    tilemap_scroll(LAYER1, 0, 0);

    // reset screen
    ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);

    zos_err_t err = zar_file_close(&zar_file);
    handle_error(err, "close zar", 0);

    err = player_deinit();
    handle_error(err, "deinit player", 0);

#if DEBUG
    for(uint8_t y = 0; y < ROOM_HEIGHT; y++) {
        for(uint8_t x = 0; x < ROOM_WIDTH; x++) {
            printf("%02x", tilemap_get_xy(x, y));
        }
        printf("\n");
    }
#endif
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

void update(void)
{
    bullet_move();
    enemies_move(&player.point);

    bool player_moved = player_move();
    player_animate();
    if(player_moved) {
        Edge map_edge = player_collide();
        if (map_edge != EdgeNone) {
            bullets_clear();
            draw();
            screen_transition(map_edge, player.point, player_set_xy);
        }
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

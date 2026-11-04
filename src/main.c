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
#include "game_world.h"
#include "player.h"
#include "bullet.h"
#include "enemies.h"

gfx_context vctx;
uint8_t buffer[WIDTH];
uint8_t game_state = GAME_TITLE;
uint8_t game_difficulty = 1;
uint32_t high_score;
static uint16_t input1_prev;
static uint8_t fire_held;
static uint8_t shot_tick;
static uint8_t death_ticks;
static void show_title(uint8_t game_over);

int main(void)
{
    init();
    uint8_t tick_phase = 0;
    while (true) {
        TSTATE_LOG(1);
        uint8_t action = input();
        if (action == ACTION_QUIT)
            goto quit_game;
        // PAL original: 50 logical ticks per second on the 60 Hz Zeal display.
        tick_phase += 50;
        if (tick_phase >= 60) {
            tick_phase -= 60;
            update();
        }
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

    err = sprites_register_arena(SPRITE_ARENA, MAX_SPRITES);
    handle_error(err, "register sprite arena", 1);

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

    err = sprites_clear(&vctx);
    handle_error(err, "clear sprites", 1);

    tilemap_fill(&vctx, LAYER0, 0, 0, 0, WIDTH, HEIGHT); // clear all layer0 memory
    tilemap_fill(&vctx, LAYER1, 0, 0, 0, WIDTH, HEIGHT);

    err = load_palette(&vctx, ZAR_TILESET_ZTP, 0);
    handle_error(err, "load palette", 1);

    // err = gfx_tileset_add_color_tile(&vctx, EMPTY_TILE, 0);
    // handle_error(err, "create empty tile", 1);

    uint16_t tile_offset        = 0;
    gfx_tileset_options options = {
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

    err = game_world_init();
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

    show_title(0);
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

static void show_title(uint8_t game_over) {
    game_state = GAME_TITLE;
    fire_held = 0;
    bullets_clear();
    enemies_clear();
    player.sprite_head->x = SCREEN_WIDTH + SPRITE_WIDTH;
    player.sprite_body->x = SCREEN_WIDTH + SPRITE_WIDTH;
    tilemap_fill(&vctx, LAYER0, 138, ROOM_OFFSET_X, ROOM_OFFSET_Y, ROOM_WIDTH, ROOM_HEIGHT);
    nprint_string_layer(&vctx, "ENIGMA", 6, LAYER0, 17, 10);
    if (game_over) nprint_string_layer(&vctx, "GAME OVER", 9, LAYER0, 15, 12);
    sprintf(buffer, "LEVEL: %d", game_difficulty);
    nprint_string_layer(&vctx, buffer, 8, LAYER0, 16, 14);
    nprint_string_layer(&vctx, "ARROWS: LEVEL  SPACE: START", 26, LAYER0, 7, 16);
}

void reset(void) {
    for (uint8_t i = 0; i < keys_len; i++) keys[i].flags = OBJECT_ACTIVE;
    for (uint8_t i = 0; i < fuel_len; i++) fuel[i].flags = OBJECT_ACTIVE;
    for (uint8_t i = 0; i < gates_len; i++) gates[i].flags |= OBJECT_ACTIVE;
    bullets_clear();
    player_reset();
    shot_tick = 0;
    fire_held = 0;
    zos_err_t err = game_world_init();
    handle_error(err, "start game", 1);
    hud_lives(player.lives);
    hud_keys(player.keys);
    hud_score(player.score);
    hud_energy(player.energy);
    game_state = GAME_PLAYING;
}

uint8_t input(void) {
    uint16_t input1 = input_read();
    uint16_t pressed = input1 & ~input1_prev;
    input1_prev = input1;
    if (SELECT1) return ACTION_QUIT;
    if (game_state == GAME_TITLE) {
        if (pressed & (BUTTON_LEFT | BUTTON_UP)) {
            game_difficulty = game_difficulty == 1 ? 4 : game_difficulty - 1;
            show_title(0);
        } else if (pressed & (BUTTON_RIGHT | BUTTON_DOWN)) {
            game_difficulty = game_difficulty == 4 ? 1 : game_difficulty + 1;
            show_title(0);
        }
        if (pressed & BUTTON_B) reset();
        return 0;
    }
    fire_held = (input1 & BUTTON_B) != 0;
    vec2_set(player.direction, DIRECTION_NONE, DIRECTION_NONE);
    if (LEFT1) player.direction.x = DIRECTION_LEFT;
    if (RIGHT1) player.direction.x = DIRECTION_RIGHT;
    if (UP1) player.direction.y = DIRECTION_UP;
    if (DOWN1) player.direction.y = DIRECTION_DOWN;
    return 0;
}

void update(void) {
    if (game_state == GAME_TITLE) return;
    if (game_state == GAME_DYING) {
        if (--death_ticks) return;
        if (!player.lives) {
            show_title(1);
        } else {
            zos_err_t err = game_world_respawn();
            handle_error(err, "respawn player", 1);
            hud_energy(player.energy);
            game_state = GAME_PLAYING;
        }
        return;
    }
    bullet_move();
    enemies_move(&player.point);
    bool player_moved = player_move();
    player_animate();
    if (player_moved) {
        Edge edge = player_collide();
        if (edge != EdgeNone) {
            bullets_clear();
            zos_err_t err = game_world_transition(edge);
            handle_error(err, "change room", 1);
            return;
        }
    }
    shot_tick ^= 1;
    if (shot_tick && fire_held && player.energy && bullet_shoot(&player.point, &player.direction)) {
        player.energy--;
        hud_energy(player.energy);
    }
    if (enemies_touch_player(&player.point)) {
        player.lives--;
        hud_lives(player.lives);
        bullets_clear();
        vec2_set(player.direction, DIRECTION_NONE, DIRECTION_NONE);
        fire_held = 0;
        game_state = GAME_DYING;
        // Brief visible death pause; the original uses a palette/sound sequence.
        death_ticks = 50;
    }
}

void draw(void)
{
    gfx_wait_vblank(&vctx);
    sprites_render(&vctx);
    gfx_wait_end_vblank(&vctx);
}

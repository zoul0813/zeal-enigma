#include <stdio.h>
#include <zgdk.h>
#include "hud.h"
#include "main.h"
#include "gates.h"
#include "player.h"

void hud_init(void)
{
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
    nprint_string_layer(&vctx, buffer, 18, LAYER0, 2, HEIGHT - 4);
    clear_tilemap(LAYER1, RED_WHITE, 9, 26, 3, 1);
    clear_tilemap(LAYER1, RED_GREY, 21, 26, 17, 1);
    for (uint8_t i = 0; i < keys_len; i++) {
        gfx_tilemap_place(&vctx, KEYS_INDEX + i, LAYER0, 21 + (i * 2), 26);
    }

    sprintf(buffer, "SCORE: %08d   HI-SCORE: %08d", 0, 0);
    nprint_string_layer(&vctx, buffer, 36, LAYER0, 2, HEIGHT - 3);
    clear_tilemap(LAYER1, RED_WHITE, 9, 27, 8, 1);
    clear_tilemap(LAYER1, RED_WHITE, 30, 27, 8, 1);

    sprintf(buffer, "ENERGY:");
    nprint_string_layer(&vctx, buffer, 7, LAYER0, 2, HEIGHT - 2);

    // energy bar, max 28 cells
    for (uint8_t i = 0; i < 28; i++) {
        gfx_tilemap_place(&vctx, 4, LAYER0, 10 + i, 28);
    }
}

void hud_score(uint32_t score)
{
    sprintf(buffer, "%08d", score);
    nprint_string_layer(&vctx, buffer, 8, LAYER0, 9, 27);

    // update hi-score?
    sprintf(buffer, "%08d", score);
    nprint_string_layer(&vctx, buffer, 8, LAYER0, 30, 27);
}

void hud_lives(uint8_t lives)
{
    char tiles[MAX_LIVES] = {0};
    for (uint8_t i = 0; i < lives; i++) {
        tiles[i] = '*';
    }
    sprintf(buffer, "%s", lives);
    nprint_string_layer(&vctx, buffer, MAX_LIVES, LAYER0, 9, 26);
}

void hud_energy(uint8_t energy)
{
    char tiles[MAX_ENERGY] = {4}; // tile for full energy
    for(uint8_t i = MAX_ENERGY; i > energy; i--) {
        tiles[i] = 3;
    }
    gfx_tilemap_load(&vctx, tiles, MAX_ENERGY, LAYER0, 10, 28);
}

void hud_keys(uint16_t keys)
{
    // clear_tilemap(LAYER1, RED_GREY, 21, 26, 17, 1);
    for (uint8_t i = 0; i < keys_len; i++) {
        if(keys & (1 << i)) {
            // we have this key?
            gfx_tilemap_place(&vctx, RED_WHITE, LAYER1, 21 + (i * 2), 26);
        }
    }
}

#include <stdio.h>
#include <zgdk/utils/print.h>
#include <zgdk/tilemap.h>
#include "hud.h"
#include "main.h"
#include "objects.h"
#include "player.h"

void hud_init(void)
{
    // sprintf(buffer, "1985 Enigma by CBM UK Ltd");
    // nprint_string_layer(&vctx, buffer, 25, LAYER0, 2, 0);

    // sprintf(buffer, "2025 Port for Zeal 8-bit Computer");
    // nprint_string_layer(&vctx, buffer, 33, LAYER0, 2, 1);

    sprintf(buffer, "Code: zoul0813 Graphics: arcadia6502");
    nprint_string_layer(&vctx, buffer, 36, LAYER0, 2, 2);

    sprintf(buffer, "Arrows: Move  Space: Shoot");
    nprint_string_layer(&vctx, buffer, 26, LAYER0, 2, 3);

    // TODO: make this a HUD update function
    sprintf(buffer, "LIVES:       KEYS:");
    nprint_string_layer(&vctx, buffer, 18, LAYER0, 2, HEIGHT - 4);
    tilemap_fill(&vctx, LAYER1, RED_WHITE, 9, 26, 3, 1);
    tilemap_fill(&vctx, LAYER1, RED_GREY, 21, 26, 17, 1);
    for (uint8_t i = 0; i < keys_len; i++) {
        gfx_tilemap_place(&vctx, KEYS_INDEX + i, LAYER0, 21 + (i * 2), 26);
    }

    sprintf(buffer, "SCORE:            HI-SCORE:");
    nprint_string_layer(&vctx, buffer, 27, LAYER0, 2, HEIGHT - 3);
    tilemap_fill(&vctx, LAYER1, RED_WHITE, 9, 27, 8, 1);
    tilemap_fill(&vctx, LAYER1, RED_WHITE, 30, 27, 8, 1);

    sprintf(buffer, "ENERGY:");
    nprint_string_layer(&vctx, buffer, 7, LAYER0, 2, HEIGHT - 2);
}

void hud_score(uint32_t score)
{
    sprintf(buffer, "%08lu", score);
    nprint_string_layer(&vctx, buffer, 8, LAYER0, 9, 27);

    if (score > high_score) high_score = score;
    sprintf(buffer, "%08lu", high_score);
    nprint_string_layer(&vctx, buffer, 8, LAYER0, 30, 27);
}

void hud_lives(uint8_t lives)
{
    for (uint8_t i = 0; i < MAX_LIVES; i++) {
        buffer[i] = i < lives ? HUD_LIFE : 138;
    }
    gfx_tilemap_load(&vctx, buffer, MAX_LIVES, LAYER0, 9, 26);
}

void hud_energy(uint8_t energy)
{
    // Retain the four-part gauge artwork while tracking the original 215 shots.
    uint8_t gauge = (uint16_t)energy * (ENERGY_BARS * ENERGY_DIVIDER) / MAX_ENERGY;
    uint8_t full = gauge / ENERGY_DIVIDER;
    uint8_t partial = gauge % ENERGY_DIVIDER;

    uint8_t i;
    for(i = 0; i < full; i++) {
        buffer[i] = HUD_ENERGY;
    }
    if(partial > 0) {
        buffer[i] = HUD_DRAINED + partial;
        i++;
    }
    for(;i<ENERGY_BARS;i++) {
        buffer[i] = HUD_DRAINED;
    }
    buffer[i] = HUD_DRAINED; // last bar

    gfx_tilemap_load(&vctx, buffer, ENERGY_BARS + 1, LAYER0, 10, 28);
}

void hud_keys(uint16_t keys)
{
    tilemap_fill(&vctx, LAYER1, RED_GREY, 21, 26, 17, 1);
    for (uint8_t i = 0; i < keys_len; i++) {
        if(keys & (1 << i)) {
            // we have this key?
            gfx_tilemap_place(&vctx, RED_WHITE, LAYER1, 21 + (i * 2), 26);
        }
    }
}

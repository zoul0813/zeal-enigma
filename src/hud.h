#include <stdint.h>

#ifndef HUD_H
#define HUD_H

#define HUD_LIFE    156
#define HUD_ENERGY  10
#define HUD_DRAINED 6

void hud_init(void);
void hud_score(uint32_t score);
void hud_lives(uint8_t lives);
void hud_energy(uint8_t energy);
void hud_keys(uint16_t keys);

#endif
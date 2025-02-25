#include <stdint.h>

#ifndef HUD_H
#define HUD_H

void hud_init(void);
void hud_score(uint32_t score);
void hud_lives(uint8_t lives);
void hud_energy(uint8_t energy);
void hud_keys(uint16_t keys);

#endif
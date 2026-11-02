#include <stdint.h>
#include <zvb_sprite.h>
#include <zgdk/types.h>

#include "main.h"

#ifndef PLAYER_H
#define PLAYER_H

#define PLAYER_SPEED (4U)

#define ENERGY_DIVIDER (4U)
#define ENERGY_BARS    (27U)
#define MAX_ENERGY     (215U)
#define MAX_LIVES      (3U)
#define MAX_SCORE      (99999999U)

typedef struct {
        Moveable(moveable);

        uint8_t lives;
        uint8_t energy;
        uint16_t keys; // bitmask of 9 keys
        uint32_t score;

        // sprites
        gfx_sprite* sprite_head;
        gfx_sprite* sprite_body;

#if DEBUG
        gfx_sprite* debug1;
        gfx_sprite* debug2;
#endif
} player_t;

extern player_t player;

error player_init(void);
error player_deinit(void);
void player_reset(void);
bool player_move(void);
void player_animate(void);
Edge player_collide(void);

void player_shoot(void);

void player_set_xy(uint16_t x, uint16_t y);
inline void player_set_point(Point *point);

#endif

#include <stdint.h>
#include <zvb_sprite.h>
#include <zgdk.h>

#include "main.h"

#ifndef PLAYER_H
#define PLAYER_H

#define PLAYER_SPEED (3U)

#define MAX_ENERGY  (28U)
#define MAX_LIVES   (3U)
#define MAX_SCORE   (99999999U)

typedef struct {
        union {
                Rect rect;
                struct {
                        Point point;
                        Size size;
                };
        };

        Direction direction;

        uint8_t lives;
        uint8_t energy;
        uint16_t keys; // bitmask of 9 keys
        uint32_t score;

        // sprites
        gfx_sprite* sprite_head;
        gfx_sprite* sprite_body;
} player_t;

extern player_t player;

error player_init(void);
error player_deinit(void);
Edge player_move(void);
void player_set_xy(uint16_t x, uint16_t y);

#endif
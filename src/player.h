#include <stdint.h>
#include <zvb_sprite.h>
#include <zgdk.h>

#include "main.h"

#ifndef PLAYER_H
#define PLAYER_H

#define PLAYER_SPEED (3U)

typedef struct {
        union {
                Rect rect;
                struct {
                        Point point;
                        Size size;
                };
        };

        Direction direction;

        uint16_t score;

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
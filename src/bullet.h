#include <stdint.h>
#include <zvb_gfx.h>
#include <zgdk/types.h>

#include "main.h"
#include "world.h"
#include "objects.h"

#ifndef BULLET_H
#define BULLET_H

#define BULLET_TILE 13
#define BULLET_INACTIVE TILE_NONE
#define MAX_BULLETS    (4U)
#define BULLET_SPEED   (5U)

typedef struct {
    Direction direction;
    gfx_sprite *sprite;
} bullet_t;

extern bullet_t BULLETS[MAX_BULLETS];

error bullet_init(void);
void bullet_move(void);
uint8_t bullet_shoot(Point *point, Direction *direction);

#endif

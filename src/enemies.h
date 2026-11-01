#include <zvb_gfx.h>
#include <zgdk/types.h>

#include "game_world.h"
#include "main.h"


#ifndef ENEMIES_H
#define ENEMIES_H

#define MAX_ENEMIES    8
#define ENEMY_INDEX    48
#define ENEMY_INACTIVE TILE_NONE
#define ENEMY_SPEED    3

typedef enum {
    ENEMY_A = 0x01,
    ENEMY_B = 0x02,
    ENEMY_C = 0x04,
    ENEMY_D = 0x08,
    ENEMY_E = 0x0F,
} EnemyType;

typedef struct {
        Moveable(moveable);

        EnemyType type;

        gfx_sprite* sprite_l;
        gfx_sprite* sprite_r;
} enemy_t;

extern enemy_t ENEMIES[MAX_ENEMIES];
extern uint8_t ENEMY_ROOMS[WORLD_WIDTH * WORLD_HEIGHT];

error enemies_init(void);
void enemies_spawn(uint8_t count);
void enemies_move(Point *player_point);

Edge enemy_collide(enemy_t *enemy);
void enemy_destroy(enemy_t *enemy);
void enemy_set_xy(enemy_t *enemy, uint16_t x, uint16_t y);
inline void enemy_set_point(enemy_t *enemy, Point *point);
#endif

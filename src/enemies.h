#include <zvb_gfx.h>
#include <zgdk/types.h>
#include <zgdk/entities.h>

#include "game_world.h"
#include "main.h"


#ifndef ENEMIES_H
#define ENEMIES_H

#define MAX_ENEMIES    16
#define ENEMY_INDEX    48
#define ENEMY_INACTIVE TILE_NONE
#define ENEMY_SPEED    3

typedef struct {
        /* Room/sprite coordinates; a horizontal pair shares one lifecycle. */
        Point point;
        uint8_t type;

        gfx_sprite* sprite_l;
        gfx_sprite* sprite_r;
} enemy_t;

extern enemy_t ENEMIES[MAX_ENEMIES];
extern entity_pool_t enemy_pool;


error enemies_init(void);
void enemies_spawn(uint8_t room_id);
void enemies_clear(void);
bool enemies_touch_player(const Point *point);
uint16_t enemy_score(const enemy_t *enemy);
void enemies_move(Point *player_point);

void enemy_destroy(enemy_t *enemy);
void enemy_set_xy(enemy_t *enemy, uint16_t x, uint16_t y);
inline void enemy_set_point(enemy_t *enemy, Point *point);
#endif

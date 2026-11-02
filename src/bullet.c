#include <zgdk/sprites.h>
#include "bullet.h"
#include "enemies.h"
#include "player.h"
#include "hud.h"

bullet_t BULLETS[MAX_BULLETS];
static uint8_t frame = 0;

error bullet_init(void)
{
    gfx_sprite sprite = {
        .tile  = BULLET_INACTIVE,
        .flags = SPRITE_NONE,
    };

    sprite.x = SCREEN_WIDTH + SPRITE_WIDTH;
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        bullet_t* bullet = &BULLETS[i];
        bullet->sprite   = sprites_register(sprite);
    }

    return GFX_SUCCESS;
}

void bullet_destroy(bullet_t *bullet) {
    bullet->sprite->x    = SCREEN_WIDTH + SPRITE_WIDTH;
    bullet->sprite->tile = BULLET_INACTIVE;
}

void bullets_clear(void) {
    for(uint8_t i = 0; i < MAX_BULLETS; i++) {
        bullet_destroy(&BULLETS[i]);
    }
}

void bullet_move(void)
{
    uint8_t i, j;
    int16_t x, y, cx, cy;
    frame++;
    if(frame > 7) frame = 0;

    for (i = 0; i < MAX_BULLETS; i++) {
        bullet_t* bullet = &BULLETS[i];
        uint8_t tile = bullet->sprite->tile;

        if (tile != TILE_NONE) {
            x = bullet->sprite->x;
            y = bullet->sprite->y;

            x += (bullet->direction.x * BULLET_SPEED);
            y += (bullet->direction.y * BULLET_SPEED);
            cx = x - SPRITE_WIDTH / 2;
            cy = y - SPRITE_HEIGHT / 2;
            if (x < (int16_t)(ROOM_LEFT_PX + SPRITE_WIDTH) || x > ROOM_RIGHT_PX ||
                y < (int16_t)(ROOM_TOP_PX + SPRITE_HEIGHT) || y > (int16_t)ROOM_BOTTOM_PX) {
                bullet_destroy(bullet);
                goto next_bullet;
            }
            int16_t type = tilemap_get_xy((cx >> 4) - ROOM_OFFSET_X, (cy >> 4) - ROOM_OFFSET_Y);
            if (type != TILE_NONE) {
                bullet_destroy(bullet);
                goto next_bullet;
            }
            for (j = 0; j < MAX_ENEMIES; j++) {
                enemy_t *enemy = &ENEMIES[j];
                if (enemy->sprite_l->tile == ENEMY_INACTIVE) continue;
                if (cx >= (int16_t)enemy->point.x - (int16_t)SPRITE_WIDTH && cx < (int16_t)enemy->point.x + (int16_t)SPRITE_WIDTH &&
                    cy >= (int16_t)enemy->point.y - (int16_t)SPRITE_HEIGHT && cy < (int16_t)enemy->point.y) {
                    player.score += enemy_score(enemy);
                    if (player.score > MAX_SCORE) player.score -= MAX_SCORE + 1UL;
                    hud_score(player.score);
                    enemy_destroy(enemy);
                    bullet_destroy(bullet);
                    goto next_bullet;
                }
            }

            if(frame == 6) tile++;
            if(tile > BULLET_TILE + 2) tile = BULLET_TILE;
            bullet->sprite->tile = tile;
            bullet->sprite->x = x;
            bullet->sprite->y = y;
        }
next_bullet:;
    }
}

uint8_t bullet_shoot(Point* point, Direction* direction)
{
    // no stationary shooting
    if(direction->x == 0 && direction->y == 0) return 0;

    uint8_t i = 0;
    for (i = 0; i < MAX_BULLETS; i++) {
        bullet_t* bullet = &BULLETS[i];
        if (bullet->sprite->tile == BULLET_INACTIVE) {
            bullet->sprite->tile = BULLET_TILE + 2;

            bullet->sprite->x = point->x;
            bullet->sprite->y = point->y;

            bullet->direction.x = direction->x;
            bullet->direction.y = direction->y;

            return 1;
        }
    }
    return 0;
}

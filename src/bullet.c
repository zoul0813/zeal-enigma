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
            cx = x + 8;
            cy = y + 8;

            if ((x > SCREEN_WIDTH || x < SPRITE_WIDTH) || (y > SCREEN_HEIGHT || y < SPRITE_HEIGHT)) {
                // offscreen
                bullet_destroy(bullet);
                goto next_bullet;
            }

            for(j = 0; j < MAX_ENEMIES; j++) {
                enemy_t *enemy = &ENEMIES[j];
                if(enemy->sprite_l->tile == ENEMY_INACTIVE) continue;
                if((cx >= enemy->sprite_l->x) && (cx <= enemy->sprite_r->x + SPRITE_WIDTH)) {
                    // inside
                    if((cy >= enemy->sprite_l->y) && (cy <= enemy->sprite_l->y + SPRITE_HEIGHT)) {
                        // hit enemy
                        uint32_t value = (enemy->sprite_l->tile - ENEMY_INDEX) + 1;
                        value <<= 10;
                        player.score += value;
                        hud_score(player.score);
                        enemy_destroy(enemy);
                        bullet_destroy(bullet);
                        goto next_bullet;
                    }
                }
            }

            // TODO: better tile x/y coordinate system?
            uint8_t tx, ty;
            tx = ((x + 8) >> 4) - 1;
            if (bullet->direction.x > 0)
                tx++;
            ty = ((y + 8) >> 4) - 1;
            if (bullet->direction.y > 0)
                ty++;

            TileType type = screen_get_tile(tx - ROOM_OFFSET_X, ty - ROOM_OFFSET_Y);
            if(type != TILE_NONE) {
                // hit something
                bullet_destroy(bullet);
                goto next_bullet;
            }

            if(frame == 6) tile++;
            if(tile > BULLET_TILE + 2) tile = BULLET_TILE;
            bullet->sprite->tile = tile;
            bullet->sprite->x = x;
            bullet->sprite->y = y;
        }
next_bullet:
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
            break;
        }
    }
    return 0;
}

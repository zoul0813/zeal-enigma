#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "gameplay_platform.h"
#include "assets.h"
#include "bullet.h"
#include "enemies.h"
#include "player.h"

uint8_t data[MAX_DATA_KB * 1024];
gfx_context vctx;
Vector2_u8 rooms_position;
uint8_t game_difficulty = 1;
static gfx_sprite arena[MAX_SPRITES];
static uint8_t registered;
static uint8_t sprite_limit = MAX_SPRITES;
static uint8_t room[ROOM_HEIGHT][ROOM_WIDTH];
static Tilemap test_map = { .tiles = &room[0][0], .rect = { .w = ROOM_WIDTH, .h = ROOM_HEIGHT } };
Tilemap *_TILEMAP = &test_map;
static uint16_t random_state = 1;
static uint32_t shown_score;

/* Real ZGDK geometry types, deterministic sprite storage and logical tilemap. */
gfx_sprite *sprites_register(gfx_sprite sprite) {
    if (registered == sprite_limit) return NULL;
    arena[registered] = sprite;
    return &arena[registered++];
}
void sprite_set_xy(gfx_sprite *sprite, uint16_t x, uint16_t y) {
    sprite->x = x; sprite->y = y;
}
uint8_t rand8(void) {
    random_state = random_state * 25173u + 13849u;
    return random_state >> 8;
}
int16_t tilemap_get_xy(uint8_t x, uint8_t y) {
    if (x >= ROOM_WIDTH || y >= ROOM_HEIGHT) return -1;
    return room[y][x];
}
int16_t tilemap_get(const Point *point) {
    return tilemap_get_xy(point->x, point->y);
}
int16_t tilemap_set_xy(uint8_t x, uint8_t y, uint8_t tile) {
    assert(x < ROOM_WIDTH && y < ROOM_HEIGHT);
    return room[y][x] = tile;
}
bool tile_get_edge(Rect *rect, Edge edge, Tile *tile) {
    (void)rect; (void)edge; (void)tile;
    return false; /* Player wall/navigation tests require the actual SDK in the target emulator. */
}
void hud_score(uint32_t score) { shown_score = score; }
void hud_energy(uint8_t energy) { assert(energy <= MAX_ENERGY); }
void hud_keys(uint16_t keys) { (void)keys; }
void room_set_tile(uint8_t x, uint8_t y, uint8_t tile) {
    tilemap_set_xy(x, y, tile);
}
extern bool player_pickup(int16_t);

static unsigned active_enemies(void) {
    unsigned count = 0;
    for (unsigned i = 0; i < MAX_ENEMIES; i++)
        count += ENEMIES[i].sprite_l->tile != ENEMY_INACTIVE;
    return count;
}
static unsigned active_bullets(void) {
    unsigned count = 0;
    for (unsigned i = 0; i < MAX_BULLETS; i++)
        count += BULLETS[i].sprite->tile != BULLET_INACTIVE;
    return count;
}
static void empty_room(void) {
    memset(room, TILE_NONE, sizeof(room));
    player_reset();
    bullets_clear();
}

static void test_original_populations_and_spawns(void) {
    empty_room();
    /* Original room zero is TMX room 56, not TMX room zero. */
    game_difficulty = 1;
    enemies_spawn(56);
    assert(active_enemies() == 9);
    enemies_spawn(0);
    assert(active_enemies() == 10);
    game_difficulty = 3;
    enemies_spawn(56);
    assert(active_enemies() == 6);
    enemies_spawn(0);
    assert(active_enemies() == 7);
    for (game_difficulty = 1; game_difficulty <= 4; game_difficulty++) {
        enemies_spawn(7); /* Original room 63 has the full sixteen enemies. */
        assert(active_enemies() == 16);
        unsigned pc = (player.point.x - 16) / 16;
        unsigned pr = (player.point.y - 16 - ROOM_TOP_PX) / 16;
        for (unsigned i = 0; i < MAX_ENEMIES; i++) {
            enemy_t *e = &ENEMIES[i];
            assert(e->type < 8);
            unsigned col = (e->point.x - 16) / 16;
            unsigned row = (e->point.y - 16 - ROOM_TOP_PX) / 16;
            assert(col / 8 != pc / 8 || row / 7 != pr / 7);
            assert(col + 1 < ROOM_WIDTH && row < ROOM_HEIGHT);
            for (unsigned j = 0; j < i; j++) {
                enemy_t *other = &ENEMIES[j];
                assert(e->point.y != other->point.y ||
                       e->point.x + 16 <= other->point.x - 16 ||
                       other->point.x + 16 <= e->point.x - 16);
            }
        }
    }
    /* Empty/small rooms terminate safely instead of retrying random spawns forever. */
    memset(room, TILE_WALL, sizeof(room));
    enemies_spawn(7);
    assert(active_enemies() == 0);
    enemies_spawn(255);
    assert(active_enemies() == 0);
}

static void test_delay_and_simultaneous_chase(void) {
    static const unsigned pauses[] = {32, 24, 16, 8};
    for (game_difficulty = 1; game_difficulty <= 4; game_difficulty++) {
        empty_room();
        enemies_spawn(27); /* Original room35: two enemies on every level. */
        assert(active_enemies() == 2);
        enemy_set_xy(&ENEMIES[0], 160, 160);
        enemy_set_xy(&ENEMIES[1], 256, 160);
        Point target = {400, 300};
        for (unsigned i = 0; i < pauses[game_difficulty - 1]; i++) {
            enemies_move(&target);
            assert(ENEMIES[0].point.x == 160 && ENEMIES[0].point.y == 160);
            assert(ENEMIES[1].point.x == 256 && ENEMIES[1].point.y == 160);
        }
        enemies_move(&target);
        unsigned step_x = game_difficulty & 1 ? 4 : 8;
        assert(ENEMIES[0].point.x == 160 + step_x && ENEMIES[0].point.y == 164);
        assert(ENEMIES[1].point.x == 256 + step_x && ENEMIES[1].point.y == 164);
        enemies_move(&target);
        assert(ENEMIES[0].point.x == 160 + 2 * step_x); /* Each call is one enemy pass. */
    }
}

static void test_walls_and_contact(void) {
    empty_room();
    game_difficulty = 1;
    enemies_spawn(27);
    enemy_destroy(&ENEMIES[1]);
    enemy_set_xy(&ENEMIES[0], 160, 160);
    /* Enemy's right edge meets column11 at x176; further X movement is blocked. */
    room[5][11] = TILE_WALL;
    Point target = {400, 220};
    for (unsigned i = 0; i < 33; i++) enemies_move(&target);
    assert(ENEMIES[0].point.x == 160);
    assert(ENEMIES[0].point.y == 164); /* Y pursuit continues beside the wall. */
    enemy_set_xy(&ENEMIES[0], 160, 160);
    assert(enemies_touch_player(&(Point){160,160}));
    assert(!enemies_touch_player(&(Point){144,160}));
    assert(enemies_touch_player(&(Point){145,160}));
    assert(!enemies_touch_player(&(Point){192,160}));
    assert(!enemies_touch_player(&(Point){160,192}));
    assert(!enemies_touch_player(&(Point){160,128}));
    enemy_destroy(&ENEMIES[0]);
    assert(!enemies_touch_player(&(Point){160,160}));
}

static void test_weapons_and_scores(void) {
    static const uint16_t awards[] = {10,20,40,80,100,200,400,800};
    empty_room();
    enemies_clear();
    Direction direction = {0,0};
    Point point = {160,160};
    assert(!bullet_shoot(&point, &direction));
    assert(active_bullets() == 0);
    direction.x = 1;
    for (unsigned i = 0; i < 8; i++) assert(bullet_shoot(&point, &direction));
    assert(active_bullets() == 8);
    assert(!bullet_shoot(&point, &direction));
    bullets_clear();
    assert(active_bullets() == 0);
    for (unsigned type = 0; type < 8; type++) {
        enemies_clear();
        enemy_t *enemy = entity_pool_alloc(&enemy_pool, type);
        assert(enemy == &ENEMIES[0]);
        enemy_set_xy(enemy, 192, 160);
        enemy->sprite_l->tile = ENEMY_INDEX + 4 * type;
        assert(enemy_score(enemy) == awards[type]);
        enemy->sprite_l->tile += 2; /* Different animation frame, same award. */
        assert(enemy_score(enemy) == awards[type]);
        point = (Point){176,160};
        player.score = 0;
        assert(bullet_shoot(&point, &direction));
        bullet_move();
        assert(enemy->sprite_l->tile == ENEMY_INACTIVE);
        assert(player.score == awards[type] && shown_score == awards[type]);
        assert(active_bullets() == 0);
    }
    // Projectiles stop at terrain, including when an enemy is directly behind it.
    enemies_clear();
    assert(entity_pool_alloc(&enemy_pool, 0) == &ENEMIES[0]);
    ENEMIES[0].sprite_l->tile = ENEMY_INDEX;
    enemy_set_xy(&ENEMIES[0], 192, 160);
    room[5][11] = TILE_WALL;
    assert(bullet_shoot(&point, &direction));
    bullet_move();
    assert(active_bullets() == 0);
    assert(ENEMIES[0].sprite_l->tile != ENEMY_INACTIVE);
    room[5][11] = TILE_NONE;
    enemies_clear();
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (!dx && !dy) continue;
            point = (Point){320,240};
            direction = (Direction){dx,dy};
            assert(bullet_shoot(&point, &direction));
            bullet_move();
            assert(BULLETS[0].sprite->x == 320 + dx * 16);
            assert(BULLETS[0].sprite->y == 240 + dy * 16);
            bullets_clear();
        }
    }
    point = (Point){160,ROOM_TOP_PX + 16};
    direction = (Direction){0,-1};
    assert(bullet_shoot(&point, &direction));
    bullet_move();
    assert(active_bullets() == 0); // HUD is outside the playable room.

}

static void test_player_reset_and_resources(void) {
    player.score = 12345; player.keys = 0x1ff; player.lives = 1; player.energy = 0;
    player.direction.x = -1;
    unsigned sprites_before = registered;
    player_reset();
    assert(registered == sprites_before);
    assert(player.score == 0 && player.keys == 0);
    assert(player.lives == 3 && player.energy == 215);
    assert(player.direction.x == 0 && player.direction.y == 0);
    assert(player.point.x == WORLD_PLAYER_X && player.point.y == WORLD_PLAYER_Y);
    rooms_position.x = fuel[0].room % WORLD_WIDTH;
    rooms_position.y = fuel[0].room / WORLD_WIDTH;
    fuel[0].flags = OBJECT_ACTIVE;
    player.energy = 210;
    assert(player_pickup(FUEL_INDEX));
    assert(player.energy == 215 && fuel[0].flags == OBJECT_INACTIVE);
    keys[0].flags = OBJECT_ACTIVE;
    player.score = 0;
    assert(player_pickup(KEYS_INDEX));
    assert(player.score == 5000 && shown_score == 5000 && player.keys == 1);
    assert(keys[0].flags == OBJECT_INACTIVE);
    for (unsigned i = 0; i < gates_len; i++)
        if (gates[i].id == 0) assert(gates[i].flags == OBJECT_INACTIVE);
    assert(!player_pickup(KEYS_INDEX + keys_len)); /* No tenth-key out-of-bounds read. */
}

/* Independent footprint oracle: all covered logical cells, including the
 * middle of an unaligned 32px enemy. Mutate cells after caching to catch stale
 * copies when a key/fuel/gate disappears. */
extern bool test_enemy_can_stand(int16_t, int16_t);
static void test_cached_collision(void) {
    empty_room();
    enemies_spawn(27);
    /* Each covered cell must block independently, especially the middle of
     * a three-column footprint that a corner-only optimization would miss. */
    for (int16_t dx = 0; dx < 16; ++dx) {
        for (int16_t dy = 0; dy < 16; ++dy) {
            int16_t x = 160 + dx, y = 160 + dy;
            assert(test_enemy_can_stand(x,y));
            for (unsigned row = (y-16-ROOM_TOP_PX)/16; row <= (y-1-ROOM_TOP_PX)/16; ++row) {
                for (unsigned col = (x-16)/16; col <= (unsigned)(x+15)/16; ++col) {
                    room[row][col] = TILE_UNKNOWN;
                    assert(!test_enemy_can_stand(x,y));
                    room[row][col] = TILE_NONE;
                    assert(test_enemy_can_stand(x,y));
                }
            }
        }
    }
    uint32_t seed = 17;
    for (unsigned n = 0; n < 20000; ++n) {
        seed = seed * 1664525u + 1013904223u;
        int16_t x = (seed >> 16) % 700 - 30;
        seed = seed * 1664525u + 1013904223u;
        int16_t y = (seed >> 16) % 520 - 30;
        unsigned cell = seed % (ROOM_WIDTH * ROOM_HEIGHT);
        room[cell / ROOM_WIDTH][cell % ROOM_WIDTH] = (seed & 0x100) ? (uint8_t)(seed >> 24) : TILE_NONE;
        bool expected = x >= ROOM_LEFT_PX + 16 && x + 16 <= ROOM_RIGHT_PX &&
                        y >= (int16_t)(ROOM_TOP_PX + 16) && y <= (int16_t)ROOM_BOTTOM_PX;
        if (expected) {
            unsigned left = (x-16-ROOM_LEFT_PX)/16, right = (x+15-ROOM_LEFT_PX)/16;
            unsigned top = (y-16-ROOM_TOP_PX)/16, bottom = (y-1-ROOM_TOP_PX)/16;
            for (unsigned row = top; row <= bottom; ++row)
                for (unsigned col = left; col <= right; ++col)
                    if (room[row][col] != TILE_NONE) expected = false;
        }
        assert(test_enemy_can_stand(x,y) == expected);
    }
}

static void test_entity_lifecycle(void) {
    empty_room();
    game_difficulty = 1;
    enemies_spawn(7);
    gfx_sprite *left = ENEMIES[7].sprite_l, *right = ENEMIES[7].sprite_r;
    enemy_destroy(&ENEMIES[7]);
    assert(ENEMIES[7].type == ENTITY_FREE);
    assert(left->tile == ENEMY_INACTIVE && right->tile == ENEMY_INACTIVE);
    enemy_t *reused = entity_pool_alloc(&enemy_pool, 3);
    assert(reused == &ENEMIES[7]);
    assert(reused->sprite_l == left && reused->sprite_r == right);
    /* Allocation tags own lifetime, independently of the displayed tile. */
    enemies_move(&player.point);
    enemies_move(&player.point);
    assert(left->tile >= ENEMY_INDEX);
    enemies_clear();
    for (unsigned i = 0; i < MAX_ENEMIES; ++i) {
        assert(ENEMIES[i].type == ENTITY_FREE);
        assert(ENEMIES[i].sprite_l->tile == ENEMY_INACTIVE);
        assert(ENEMIES[i].sprite_r->tile == ENEMY_INACTIVE);
    }
}

static void test_sprite_exhaustion(void) {
    for (sprite_limit = 0; sprite_limit < 2; ++sprite_limit) {
        registered = 0;
        assert(player_init() == ERR_NO_MORE_MEMORY);
    }
    for (sprite_limit = 0; sprite_limit < MAX_BULLETS; ++sprite_limit) {
        registered = 0;
        assert(bullet_init() == ERR_NO_MORE_MEMORY);
    }
    for (sprite_limit = 0; sprite_limit < 2 * MAX_ENEMIES; ++sprite_limit) {
        registered = 0;
        assert(enemies_init() == ERR_NO_MORE_MEMORY);
    }
    sprite_limit = MAX_SPRITES;
    registered = 0;
}

int main(void) {
    test_sprite_exhaustion();
    assert(player_init() == ERR_SUCCESS);
    assert(bullet_init() == ERR_SUCCESS);
    assert(enemies_init() == ERR_SUCCESS);
    assert(registered == 42);
    test_original_populations_and_spawns();
    test_delay_and_simultaneous_chase();
    test_walls_and_contact();
    test_weapons_and_scores();
    test_player_reset_and_resources();
    test_cached_collision();
    test_entity_lifecycle();
    puts("PASS: original populations/difficulty, delayed simultaneous pursuit, walls/contact, weapons, scores, reset/pickups, 20,000 cached footprints, sprite exhaustion and two-sprite entity reuse");
    return 0;
}

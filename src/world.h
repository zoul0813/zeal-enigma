#include <zgdk.h>

#include "main.h"

#ifndef WORLD_H
#define WORLD_H

#define WORLD_WIDTH    8
#define WORLD_HEIGHT   8
#define ROOM_WIDTH     WIDTH
#define ROOM_HEIGHT    21
#define ROOM_OFFSET_X  0
#define ROOM_OFFSET_Y  0
#define TILEMAP_OFFSET 0

#define WORLD_START_X  0
#define WORLD_START_Y  0
#define WORLD_PLAYER_X (SCREEN_WIDTH / 2)
#define WORLD_PLAYER_Y (SCREEN_HEIGHT / 2)

typedef void (*transition_callback_t)(uint16_t x, uint16_t y);

extern Vector2_u8 world_position;

typedef enum {
    TILE_OPEN    = 0,   // player can move on this
    TILE_WALL    = 1,   // wall, or other obstruction
    TILE_UNKNOWN = 255, // unknown, assume an obstruction
} TileType;

extern TileType SCREEN[ROOM_HEIGHT][ROOM_WIDTH];
// extern TileType SCREEN[HEIGHT][WIDTH];

error world_init(gfx_context* ctx);
void world_set_tile(uint8_t x, uint8_t y, uint8_t tile);

void screen_load(const Vector2_u8 world, const Edge edge, const Point point, transition_callback_t const callback);
uint8_t screen_transition(const Edge edge, const Point point, const transition_callback_t callback);
TileType screen_get_tile(uint8_t x, uint8_t y);

#endif

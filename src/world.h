#include <zgdk.h>

#include "main.h"

#ifndef WORLD_H
#define WORLD_H

// size of world in rooms
#define WORLD_WIDTH    8
#define WORLD_HEIGHT   8

// size of a room
#define ROOM_WIDTH     WIDTH
#define ROOM_HEIGHT    21

// tile coordinates for where the map starts
#define ROOM_OFFSET_X  0
#define ROOM_OFFSET_Y  4

// absolute offsets for the room edges
#define ROOM_LEFT_PX   (0)
#define ROOM_RIGHT_PX  (SCREEN_WIDTH)
#define ROOM_TOP_PX    (ROOM_OFFSET_Y * SPRITE_HEIGHT)
#define ROOM_BOTTOM_PX (ROOM_TOP_PX + (ROOM_HEIGHT * SPRITE_HEIGHT))

// offset for where the map starts in the zar
#define TILEMAP_OFFSET 0

// starting room
#define WORLD_START_X  3
#define WORLD_START_Y  3

// starting position of player on screen
#define WORLD_PLAYER_X ((WIDTH / 4) * SPRITE_WIDTH)
#define WORLD_PLAYER_Y (((HEIGHT / 4) * 3) * SPRITE_HEIGHT)

#define ROOM_ID() ((world_position.y * WORLD_WIDTH) + world_position.x)

typedef void (*transition_callback_t)(uint16_t x, uint16_t y);

extern Vector2_u8 world_position;

typedef enum {
    TILE_NONE    = 0,   // player can move on this
    TILE_WALL    = 1,   // wall, or other obstruction
    TILE_KEY     = 2,
    TILE_GATE    = 3,
    TILE_UNKNOWN = 255, // unknown, assume an obstruction
} TileType;

extern TileType SCREEN[ROOM_HEIGHT][ROOM_WIDTH];

error world_init(gfx_context* ctx);
void room_set_tile(const uint8_t x, const uint8_t y, const uint8_t tile);

void screen_load(const Vector2_u8 world, const Edge edge, const Point point, transition_callback_t const callback);
uint8_t screen_transition(const Edge edge, const Point point, const transition_callback_t callback);
TileType screen_get_tile(const uint8_t x, const uint8_t y);

#endif

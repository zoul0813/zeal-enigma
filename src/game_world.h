#include <zgdk/types.h>
#include <zgdk/tilemap.h>
#include <world/world.h>
#include "main.h"

#ifndef ENIGMA_GAME_WORLD_H
#define ENIGMA_GAME_WORLD_H

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

// Original (3,4), with the TMX map's vertically reversed world coordinates.
#define WORLD_START_X  3
#define WORLD_START_Y  3

// Original entry at character (6,15), scaled to the Zeal tiles and HUD.
#define WORLD_PLAYER_X (7 * SPRITE_WIDTH)
#define WORLD_PLAYER_Y ((ROOM_OFFSET_Y + 16) * SPRITE_HEIGHT)

#define ROOM_ID() ((world_position.y * WORLD_WIDTH) + world_position.x)

typedef enum {
    TILE_NONE    = 0,   // player can move on this
    TILE_WALL    = 1,   // wall, or other obstruction
    TILE_KEY     = 2,
    TILE_GATE    = 3,
    TILE_UNKNOWN = 255, // unknown, assume an obstruction
} TileType;

zos_err_t game_world_init(void);
zos_err_t game_world_respawn(void);
void room_set_tile(const uint8_t x, const uint8_t y, const uint8_t tile);

zos_err_t game_world_transition(Edge edge);

#endif

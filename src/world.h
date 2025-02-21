#include <zgdk.h>

#include "main.h"

#ifndef WORLD_H
#define WORLD_H

#define WORLD_WIDTH  16
#define WORLD_HEIGHT 8
#define SCREEN_TILES (20 * 15)

extern Vector2_u8 world_position;

typedef enum {
    TILE_OPEN     = 0,    // player can move on this
    TILE_WALL     = 1,    // wall, or other obstruction
    TILE_WATER    = 2,    // water, an obstruction for now
    TILE_CHEST    = 16,   // interactive chests
    TILE_UNKNOWN  = 255,  // unknown, assume an obstruction
} TileType;

// extern uint8_t TILEMAP[SCREEN_TILES];
extern TileType SCREEN[SCREEN_HEIGHT][SCREEN_WIDTH];

error world_init(gfx_context* ctx);
void world_set_tile(uint8_t x, uint8_t y, uint8_t tile);

void screen_load(Vector2_u8 world, const Edge edge);
uint8_t screen_transition(Edge edge);
TileType screen_get_tile(uint8_t x, uint8_t y);

#endif